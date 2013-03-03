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
	addi	sp,sp,-PT_SIZE; \
	sw		(sp+PT_RA), ra; \
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

.macro current_thread_info reg
	xnori \reg, r0, (THREAD_SIZE - 1)
	and \reg, sp, \reg
.endm

.macro switch_to_kernel_mode
	/*
	 * Store away r9,r10,r11 so that we can use it here. The tricky part is that we
	 * need to do this without clobbering any other registers. The r0 register
	 * is supposed to be always 0. Since we are running with interrupts we can
	 * allow ourselves to temporarily change it's value. Note though that r0 is
	 * also used in pseudo instructions like 'mv', so we need to restore it
	 * immediately afterwards.
	 */
	mvhi r0, hi(lm32_state)
	ori r0, r0, lo(lm32_state)
	sw (r0+STATE_SAVED_R9), r9
	sw (r0+STATE_SAVED_R10), r10
	sw (r0+STATE_SAVED_R11), r11
	mv r9, r0 /* mv is 'or rX, rY, r0', so this works */
	xor r0, r0, r0

	/*
	 * store the current kernel_mode value to the stack frame and set
	 * kernel_mode to 1
	 */
	lw r10, (r9+STATE_KERNEL_MODE)

	mv r11, sp

	bne r10, r0, 1f

	lw sp, (r9+STATE_CURRENT_THREAD)
	addi sp, sp, THREAD_SIZE - 4
1:/* already on kernel stack */

	addi sp, sp, -PT_SIZE

	/* save pt_mode, stack pointer and ra in current stack frame */
	sw (sp+PT_MODE), r10
	sw (sp+PT_SP), r11
	sw (sp+PT_RA), ra

	mvi r10, PT_MODE_KERNEL
	sw (r9+STATE_KERNEL_MODE), r10

	lw r11, (r9+STATE_SAVED_R11)
	lw r10, (r9+STATE_SAVED_R10)
	lw r9, (r9+STATE_SAVED_R9)
.endm

.macro switch_to_user_mode
	rcsr r5, IE
	andi r6, r5, 0xfffe
	wcsr IE, r6

	lw r5, (sp+PT_MODE)
	bne r5, r0, 3f
1:
	current_thread_info r3
	lw r3, (r3+TI_FLAGS)

	andi r4, r3, _TIF_WORK_MASK
	be r4, r0, 3f

	andi r4, r3, _TIF_NEED_RESCHED
	be r4, r0, 2f
	calli schedule
	bi 1b
2:
	addi r1, sp, 4
	calli do_notify_resume
	bi 1b
3:
	rcsr r5, IE
	andi r6, r5, 0xfffe
	wcsr IE, r6

	lw r2, (sp+PT_MODE)
	mvhi r1, hi(lm32_state)
	ori r1, r1, lo(lm32_state)
	sw (r1+STATE_KERNEL_MODE), r2
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
	sw (sp+PT_R1), r1 /* store return value into pt_regs */
	mvi r2, 1 /* In syscall */
	bi      _restore_and_return_exception

.badsyscall:
	mvi r1, -ENOSYS
	bi syscall_tail

/**************************/
/* exception return paths */
/**************************/

/* return path for debug or non-debug exceptions */
#define EXCEPTION_RETURN_PATH(label, branch_to) \
label: \
	/* store pt_regs* in r2 */ \
	addi      r2,  sp, 4; \
	/* store 0 into r8 (syscall no) in pt_regs */ \
	sw (sp+PT_R8), r0; \
	sw (sp+PT_R1), r1; /* store return value into pt_regs */ \
	mvi r2, 0; /* Not in syscall */ \
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

.macro save_irq_frame
	sw      (sp+PT_R1),  r1
	sw      (sp+PT_R2),  r2
	sw      (sp+PT_R3),  r3
	sw      (sp+PT_R4),  r4
	sw      (sp+PT_R5),  r5
	sw      (sp+PT_R6),  r6
	sw      (sp+PT_R7),  r7
	sw      (sp+PT_R8),  r8
	sw      (sp+PT_R9),  r9
	sw      (sp+PT_R10), r10
	/* ra (sp + PT_RA) has already been written */
	sw      (sp+PT_EA),  ea
.endm

/* restore all caller saved registers saved in save_irq_frame */
.macro restore_irq_frame
	lw      r1,  (sp+PT_R1);
	lw      r2,  (sp+PT_R2);
	lw      r3,  (sp+PT_R3);
	lw      r4,  (sp+PT_R4);
	lw      r5,  (sp+PT_R5);
	lw      r6,  (sp+PT_R6);
	lw      r7,  (sp+PT_R7);
	lw      r8,  (sp+PT_R8);
	lw      r9,  (sp+PT_R9);
	lw      r10, (sp+PT_R10);
	lw      ra,  (sp+PT_RA)
	lw      ea,  (sp+PT_EA)
	lw      sp,  (sp+PT_SP)
.endm

/* in IRQ we call a function between save and restore */
/* we therefore only save and restore the caller saved registers */
/* (r1-r10, ra, ea because an interrupt could interrupt another one) */
_long_interrupt_handler:
	switch_to_kernel_mode
	save_irq_frame

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
6:
	mvi r2, 0 /* Not in syscall */
	switch_to_user_mode
	restore_irq_frame
	eret

_save_syscall_frame:
	save_irq_frame
	sw      (sp+PT_R11), r11
	sw      (sp+PT_R12), r12
	sw      (sp+PT_R13), r13
	sw      (sp+PT_R14), r14
	sw      (sp+PT_R15), r15
	sw      (sp+PT_R16), r16
	sw      (sp+PT_R17), r17
	sw      (sp+PT_R18), r18
	sw      (sp+PT_R19), r19
	sw      (sp+PT_R20), r20
	sw      (sp+PT_R21), r21
	sw      (sp+PT_R22), r22
	sw      (sp+PT_R23), r23
	sw      (sp+PT_R24), r24
	sw      (sp+PT_R25), r25
	sw      (sp+PT_GP), r26
	sw      (sp+PT_FP), r27
	/* ra (sp + PT_RA) has already been written */
	sw      (sp+PT_BA), ba
	sw      (sp+PT_ORIG_R1),  r1

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
	lw      r11, (sp+PT_R11); \
	lw      r12, (sp+PT_R12); \
	lw      r13, (sp+PT_R13); \
	lw      r14, (sp+PT_R14); \
	lw      r15, (sp+PT_R15); \
	lw      r16, (sp+PT_R16); \
	lw      r17, (sp+PT_R17); \
	lw      r18, (sp+PT_R18); \
	lw      r19, (sp+PT_R19); \
	lw      r20, (sp+PT_R20); \
	lw      r21, (sp+PT_R21); \
	lw      r22, (sp+PT_R22); \
	lw      r23, (sp+PT_R23); \
	lw      r24, (sp+PT_R24); \
	lw      r25, (sp+PT_R25); \
	lw      r26, (sp+PT_GP); \
	lw      r27, (sp+PT_FP); \
	lw      ra,  (sp+PT_RA); \
	lw      ba,  (sp+PT_BA); \
	restore_irq_frame; \
	/* scall stores pc into ea/ba register, not pc+4, so we have to add 4 */ \
	addi	addr_register, addr_register, 4; \
	return_instr

RETURN_FROM_SYSCALL_OR_EXCEPTION(_restore_and_return_exception,ea,eret)

/*
 * struct task_struct* switch_to(struct task_struct* prev,
 *		struct thread_info *prev_ti, struct thread_info *next_ti)
 * Returns the previous task
 */
ENTRY(_switch_to)

	/* r1 gets passed through unmodified */

	sw  (r2+TI_CC_R11), r11
	sw  (r2+TI_CC_R12), r12
	sw  (r2+TI_CC_R13), r13
	sw  (r2+TI_CC_R14), r14
	sw  (r2+TI_CC_R15), r15
	sw  (r2+TI_CC_R16), r16
	sw  (r2+TI_CC_R17), r17
	sw  (r2+TI_CC_R18), r18
	sw  (r2+TI_CC_R19), r19
	sw  (r2+TI_CC_R20), r20
	sw  (r2+TI_CC_R21), r21
	sw  (r2+TI_CC_R22), r22
	sw  (r2+TI_CC_R23), r23
	sw  (r2+TI_CC_R24), r24
	sw  (r2+TI_CC_R25), r25
	sw  (r2+TI_CC_GP), r26
	sw  (r2+TI_CC_FP), r27
	sw  (r2+TI_CC_SP), sp
	sw  (r2+TI_CC_RA), ra
	sw  (r2+TI_CC_EA), ea
	sw  (r2+TI_CC_BA), ba

	mvhi r4, hi(lm32_state)
	ori r4, r4, lo(lm32_state)
	sw (r4+STATE_CURRENT_THREAD), r3

	/* restore next */
	lw  r11, (r3+TI_CC_R11)
	lw  r12, (r3+TI_CC_R12)
	lw  r13, (r3+TI_CC_R13)
	lw  r14, (r3+TI_CC_R14)
	lw  r15, (r3+TI_CC_R15)
	lw  r16, (r3+TI_CC_R16)
	lw  r17, (r3+TI_CC_R17)
	lw  r18, (r3+TI_CC_R18)
	lw  r19, (r3+TI_CC_R19)
	lw  r20, (r3+TI_CC_R20)
	lw  r21, (r3+TI_CC_R21)
	lw  r22, (r3+TI_CC_R22)
	lw  r23, (r3+TI_CC_R23)
	lw  r24, (r3+TI_CC_R24)
	lw  r25, (r3+TI_CC_R25)
	lw  r26, (r3+TI_CC_GP)
	lw  r27, (r3+TI_CC_FP)
	lw  sp,  (r3+TI_CC_SP)
	lw  ra,  (r3+TI_CC_RA)
	lw  ea,  (r3+TI_CC_EA)
	lw  ba,  (r3+TI_CC_BA)

	ret
