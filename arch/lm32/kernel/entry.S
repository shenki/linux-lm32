#include <linux/sys.h>
#include <linux/linkage.h>
#include <asm/traps.h>
#include <asm/unistd.h>
#include <asm/thread_info.h>
#include <asm/errno.h>
#include <asm/setup.h>
#include <asm/segment.h>
#include <asm/asm-offsets.h>
#include <asm/ptrace.h>

/* 
 * Exception vector table (see "LatticeMico32 Processor Reference Manual")
 */

/* exception vector for os-aware gdb and kernel signals */
#define KERNEL_EXCEPTION_VECTOR(offset) \
	addi	sp,sp,-132; \
	sw		(sp+120), ra; \
	calli	_save_syscall_frame; \
	mvi		r1, offset; \
	addi	r2, sp, 4; \
	calli	asm_do_sig; \
	bi		_return_from_exception; \
	nop

.section ".exception.text"	,"ax"

ENTRY(reset_handler)
	KERNEL_EXCEPTION_VECTOR(0)

ENTRY(breakpoint_handler)
	bret
	nop
	nop
	nop

	nop
	nop
	nop
	nop

ENTRY(instruction_bus_error_handler)
	KERNEL_EXCEPTION_VECTOR(64)

ENTRY(watchpoint_handler)
	KERNEL_EXCEPTION_VECTOR(96)

ENTRY(data_bus_error_handler)
	KERNEL_EXCEPTION_VECTOR(128)

ENTRY(divide_by_zero_handler)
	KERNEL_EXCEPTION_VECTOR(160)

ENTRY(interrupt_handler)
	bi      _long_interrupt_handler
	nop
	nop
	nop
	nop
	nop
	nop
	nop

.macro switch_to_kernel_mode
	/*
	 * Store away r9,r10 so that we can use it here. The tricky part is that we
	 * need to do this without clobbering any other registers. The r0 register
	 * is supposed to be always 0. Since we are running with interrupts we can
	 * allow ourselves to temporarily change it's value. Note though that r0 is
	 * also used in pseudo instructions like 'mv', so we need to restore it
	 * immediately afterwards.
	 */
	mvhi r0, hi(saved_state)
	ori r0, r0, lo(saved_state)
	sw (r0+0), r9
	sw (r0+4), r10
	sw (r0+8), r11
	xor r0, r0, r0

	/* test if already on kernel stack: test current_thread_info->task->which_stack */
	mvhi r9, hi(lm32_current_thread)
	ori r9, r9, lo(lm32_current_thread)
	lw r9, (r9+0) /* dereference lm32_current_thread */
	lw r10, (r9+TI_TASK) /* load pointer to task */
	lw r9, (r10+TASK_WHICH_STACK)

	mv r11, sp

	be r9, r0, 1f

	/* we are on user stack, have to switch */
	sw (r10+TASK_USP), sp /* store usp */
	lw sp, (r10+TASK_KSP) /* load ksp */
	sw (r10+TASK_WHICH_STACK), r0 /* set which_stack to 0 */

1:/* already on kernel stack */

	addi sp, sp, -132

	/*
	 * store the current kernel_mode value to the stack frame and set
	 * kernel_mode to 1
	 */
	mvhi r9, hi(kernel_mode)
	ori r9, r9, lo(kernel_mode)
	lw r10, (r9+0)
	sw (sp+132), r10
	mvi r10, PT_MODE_KERNEL
	sw (r9+0), r10

	/* save stack pointer and ra in current stack frame */
	sw (sp+116), r11
	sw (sp+120), ra

	/* restore r9,r10,r11 */
	mvhi r11, hi(saved_state)
	ori r11, r11, lo(saved_state)
	lw r9, (r11+0)
	lw r10, (r11+4)
	lw r11, (r11+8)
.endm

.macro switch_to_user_mode
	rcsr r2, IE
	andi r3, r2, 0xfffe
	wcsr IE, r3
	lw r2, (sp+132)
	mvhi r1, hi(kernel_mode)
	ori r1, r1, lo(kernel_mode)
	sw (r1+0), r2
	/* prepare switch to user stack but keep kernel stack pointer in r11 */
	/* r9: scratch register */
	/* r10: current = current_thread_info()->task */
	/* r11: ksp backup */
	/* setup r10 = current */
	bne r2, r0, 1f
	mvhi r9, hi(lm32_current_thread);
	ori r9, r9, lo(lm32_current_thread);
	lw r9, (r9+0); /* dereference lm32_current_thread */
	lw r10, (r9+TI_TASK); /* load pointer to task */
	/* set task->thread.which_stack to 1 (user stack) */
	mvi r9, TASK_USP - TASK_KSP
	sw (r10+TASK_WHICH_STACK), r9
	/* store ksp (after restore of frame) into task->thread.ksp */
	addi r9, sp, 132
	sw (r10+TASK_KSP), r9
	/* save sp into r11 */
	/* get usp into sp*/
	1:
.endm

ENTRY(system_call)
	switch_to_kernel_mode
	/* save registers */
	calli _save_syscall_frame

	rcsr r11, IE
	ori r11, r11, 1
	wcsr IE, r11

	/* r8 always holds the syscall number */
	/* check if syscall number is valid */
	mvi r9, __NR_syscalls
	bgeu r8, r9, .badsyscall
	mvhi r9, hi(sys_call_table) /* load address of syscall table */
	ori r9, r9, lo(sys_call_table)
	sli r10, r8, 2 /* TODO: only works with shifter enabled */
	add r9, r9, r10 /* add offset of syscall no to address */
	lw r9, (r9+0) /* fetch address of syscall function */
	call r9 /* execute syscall */

ENTRY(syscall_tail)
	/* store pt_regs* in r2 */
	addi      r2,  sp, 4
	calli manage_signals
	sw (sp+8), r1 /* store return value into pt_regs */

	bi      _restore_and_return_exception

.badsyscall:
	mvi r1, -ENOSYS
	sw (sp+8), r1 /* store return value into pt_regs */

	bi      _restore_and_return_exception

saved_state:
	.word 0x00
	.word 0x00
	.word 0x00

/**************************/
/* exception return paths */
/**************************/

/* return path for debug or non-debug exceptions */
#define EXCEPTION_RETURN_PATH(label, branch_to) \
label: \
	/* store pt_regs* in r2 */ \
	addi      r2,  sp, 4; \
	/* store 0 into r8 (syscall no) in pt_regs */ \
	sw (sp+36), r0; \
	calli manage_signals; \
	sw (sp+8), r1; /* store return value into pt_regs */ \
	bi branch_to

EXCEPTION_RETURN_PATH(_return_from_exception, _restore_and_return_exception)

/* ret_from_fork(arg1, arg2, continuation) */
/* calls schedule_tail and then manage_signals */
/* returns to continuation(arg1, arg2) */
ENTRY(ret_from_fork)
	calli	schedule_tail
	mvi r1, 0
	bi syscall_tail

/* ret_from_fork(arg1, arg2, continuation) */
/* calls schedule_tail and then manage_signals */
/* returns to continuation(arg1, arg2) */
ENTRY(ret_from_kernel_thread)
	calli	schedule_tail
	mv r1, r12
	mvhi ra, hi(syscall_tail)
	ori ra, ra, lo(syscall_tail)
	b r11

/* in IRQ we call a function between save and restore */
/* we therefore only save and restore the caller saved registers */
/* (r1-r10, ra, ea because an interrupt could interrupt another one) */
_long_interrupt_handler:
	switch_to_kernel_mode
	calli   _save_irq_frame

	/* Workaround hardware hazard. Sometimes the interrupt handler is entered
	 * although interrupts are disabled */
	rcsr	r1, IE
	andi	r1, r1, 0x2
	be		r1, r0, 6f

	rcsr    r3, IP
	rcsr    r4, IM
	mvi     r1, 0
	and     r3, r3, r4
	be      r3, r0, 5f

	andi	r4, r3, 0xffff
	bne		r4, r0, 1f
	sri		r3, r3, 16
	addi	r1, r1, 16
1:
	andi	r4, r3, 0xff
	bne		r4, r0, 2f
	sri		r3, r3, 8
	addi	r1, r1, 8
2:
	andi	r4, r3, 0xf
	bne		r4, r0, 3f
	sri		r3, r3, 4
	addi	r1, r1, 4
3:
	andi	r4, r3, 0x3
	bne		r4, r0, 4f
	sri		r3, r3, 2
	addi	r1, r1, 2
4:
	andi	r4, r3, 0x1
	bne		r4, r0, 5f
	addi	r1, r1, 1
5:

	addi    r2, sp, 4
	calli   asm_do_IRQ
	addi    r1, sp, 4
	calli   manage_signals_irq
6:
	bi      _restore_irq_frame_and_return

_save_irq_frame:
	sw      (sp+8),   r1
	sw      (sp+12),  r2
	sw      (sp+16),  r3
	sw      (sp+20),  r4
	sw      (sp+24),  r5
	sw      (sp+28),  r6
	sw      (sp+32),  r7
	sw      (sp+36),  r8
	sw      (sp+40),  r9
	sw      (sp+44),  r10
	/* ra (sp + 120) has already been written */
	sw      (sp+124), ea

ret

/* restore all caller saved registers saved in _save_irq_frame and return from exception */
_restore_irq_frame_and_return:
	switch_to_user_mode
	lw      r1,  (sp+8);
	lw      r2,  (sp+12);
	lw      r3,  (sp+16);
	lw      r4,  (sp+20);
	lw      r5,  (sp+24);
	lw      r6,  (sp+28);
	lw      r7,  (sp+32);
	lw      r8,  (sp+36);
	lw      r9,  (sp+40);
	lw      r10, (sp+44);
	lw      ra,  (sp+120)
	lw      ea,  (sp+124)
	lw      sp,  (sp+116)
	eret

_save_syscall_frame:
	sw      (sp+8),   r1
	sw      (sp+12),  r2
	sw      (sp+16),  r3
	sw      (sp+20),  r4
	sw      (sp+24),  r5
	sw      (sp+28),  r6
	sw      (sp+32),  r7
	sw      (sp+36),  r8
	sw      (sp+40),  r9
	sw      (sp+44),  r10
	sw      (sp+48),  r11
	sw      (sp+52),  r12
	sw      (sp+56),  r13
	sw      (sp+60),  r14
	sw      (sp+64),  r15
	sw      (sp+68),  r16
	sw      (sp+72),  r17
	sw      (sp+76),  r18
	sw      (sp+80),  r19
	sw      (sp+84),  r20
	sw      (sp+88),  r21
	sw      (sp+92),  r22
	sw      (sp+96),  r23
	sw      (sp+100), r24
	sw      (sp+104), r25
	sw      (sp+108), r26
	sw      (sp+112), r27
	/* ra (sp + 120) has already been written */
	sw      (sp+124), ea
	sw      (sp+128), ba

	ret

/************************/
/* syscall return paths */
/************************/


/* Restore all registers from syscall */
/* all interrupts are disabled upon entry */
/* we are on the kernel stack upon entry */

#define RETURN_FROM_SYSCALL_OR_EXCEPTION(label, addr_register, return_instr) \
label: \
	switch_to_user_mode; \
	/* restore frame from original kernel stack */ \
	/* restore r1 as the return value is stored onto the stack */ \
	lw      r1,  (sp+8); \
	lw      r2,  (sp+12); \
	lw      r3,  (sp+16); \
	lw      r4,  (sp+20); \
	lw      r5,  (sp+24); \
	lw      r6,  (sp+28); \
	lw      r7,  (sp+32); \
	lw      r8,  (sp+36); \
	lw      r9,  (sp+40); \
	lw      r10, (sp+44); \
	lw      r11, (sp+48); \
	lw      r12, (sp+52); \
	lw      r13, (sp+56); \
	lw      r14, (sp+60); \
	lw      r15, (sp+64); \
	lw      r16, (sp+68); \
	lw      r17, (sp+72); \
	lw      r18, (sp+76); \
	lw      r19, (sp+80); \
	lw      r20, (sp+84); \
	lw      r21, (sp+88); \
	lw      r22, (sp+92); \
	lw      r23, (sp+96); \
	lw      r24, (sp+100); \
	lw      r25, (sp+104); \
	lw      r26, (sp+108); \
	lw      r27, (sp+112); \
	lw      ra,  (sp+120); \
	lw      ea,  (sp+124); \
	lw      ba,  (sp+128); \
	lw      sp,  (sp+116); \
	/* scall stores pc into ea/ba register, not pc+4, so we have to add 4 */ \
	addi	addr_register, addr_register, 4; \
	return_instr

RETURN_FROM_SYSCALL_OR_EXCEPTION(_restore_and_return_exception,ea,eret)

/*
 * struct task_struct* resume(struct task_struct* prev, struct task_struct* next)
 * Returns the previous task
 */
ENTRY(resume)
	/* store whole state to current stack (may be usp or ksp) */
	addi sp, sp, -132
	sw  (sp+16),  r3
	sw  (sp+20),  r4
	sw  (sp+24),  r5
	sw  (sp+28),  r6
	sw  (sp+32),  r7
	sw  (sp+36),  r8
	sw  (sp+40),  r9
	sw  (sp+44),  r10
	sw  (sp+48),  r11
	sw  (sp+52),  r12
	sw  (sp+56),  r13
	sw  (sp+60),  r14
	sw  (sp+64),  r15
	sw  (sp+68),  r16
	sw  (sp+72),  r17
	sw  (sp+76),  r18
	sw  (sp+80),  r19
	sw  (sp+84),  r20
	sw  (sp+88),  r21
	sw  (sp+92),  r22
	sw  (sp+96),  r23
	sw  (sp+100), r24
	sw  (sp+104), r25
	sw  (sp+108), r26
	sw  (sp+112), r27
	addi r3, sp, 132 /* special case for stack pointer */
	sw  (sp+116), r3 /* special case for stack pointer */
	sw	(sp+120), ra
/*	sw  (sp+124), ea
	sw  (sp+128), ba */


	sw  (r1 + TASK_KSP), sp

	/* restore next */

	lw  sp, (r2 + TASK_KSP)

	lw  r3,  (sp+16)
	lw  r4,  (sp+20)
	lw  r5,  (sp+24)
	lw  r6,  (sp+28)
	lw  r7,  (sp+32)
	lw  r8,  (sp+36)
	lw  r9,  (sp+40)
	lw  r10, (sp+44)
	lw  r11, (sp+48)
	lw  r12, (sp+52)
	lw  r13, (sp+56)
	lw  r14, (sp+60)
	lw  r15, (sp+64)
	lw  r16, (sp+68)
	lw  r17, (sp+72)
	lw  r18, (sp+76)
	lw  r19, (sp+80)
	lw  r20, (sp+84)
	lw  r21, (sp+88)
	lw  r22, (sp+92)
	lw  r23, (sp+96)
	lw  r24, (sp+100)
	lw  r25, (sp+104)
	lw  r26, (sp+108)
	lw  r27, (sp+112)
	/* skip sp for now */
	lw  ra,  (sp+120)
/*	lw  ea,  (sp+124)
	lw  ba,  (sp+128) */
	/* Stack pointer must be restored last --- it will be updated */
	lw  sp,  (sp+116)

	ret
