/*
 * This program is used to generate definitions needed by
 * assembly language modules.
 *
 * We use the technique used in the OSF Mach kernel code:
 * generate asm statements containing #defines,
 * compile this file to assembler, and then extract the
 * #defines from the assembly-language output.
 */

#define ASM_OFFSETS_C

#include <linux/kbuild.h>

#include <linux/sched.h>
#include <asm/processor.h>
#include <asm/thread_info.h>

int main(void)
{
	DEFINE(TASK_STATE, offsetof(struct task_struct, state));
	DEFINE(TASK_FLAGS, offsetof(struct task_struct, flags));
	DEFINE(TASK_PTRACE, offsetof(struct task_struct, ptrace));
	DEFINE(TASK_BLOCKED, offsetof(struct task_struct, blocked));
	DEFINE(TASK_THREAD, offsetof(struct task_struct, thread));
	DEFINE(TASK_THREAD_INFO, offsetof(struct task_struct, stack));
	DEFINE(TASK_MM, offsetof(struct task_struct, mm));
	DEFINE(TASK_ACTIVE_MM, offsetof(struct task_struct, active_mm));

	DEFINE(TASK_KSP, offsetof(struct task_struct, thread.ksp));
	DEFINE(TASK_USP, offsetof(struct task_struct, thread.usp));
	DEFINE(TASK_WHICH_STACK, offsetof(struct task_struct, thread.which_stack));

	DEFINE(PT_R0, offsetof(struct pt_regs, r0));
	DEFINE(PT_R1, offsetof(struct pt_regs, r1));
	DEFINE(PT_R2, offsetof(struct pt_regs, r2));
	DEFINE(PT_R3, offsetof(struct pt_regs, r3));
	DEFINE(PT_R4, offsetof(struct pt_regs, r4));
	DEFINE(PT_R5, offsetof(struct pt_regs, r5));
	DEFINE(PT_R6, offsetof(struct pt_regs, r6));
	DEFINE(PT_R7, offsetof(struct pt_regs, r7));
	DEFINE(PT_R8, offsetof(struct pt_regs, r8));
	DEFINE(PT_R9, offsetof(struct pt_regs, r9));
	DEFINE(PT_R10, offsetof(struct pt_regs, r10));
	DEFINE(PT_R11, offsetof(struct pt_regs, r11));
	DEFINE(PT_R12, offsetof(struct pt_regs, r12));
	DEFINE(PT_R13, offsetof(struct pt_regs, r13));
	DEFINE(PT_R14, offsetof(struct pt_regs, r14));
	DEFINE(PT_R15, offsetof(struct pt_regs, r15));
	DEFINE(PT_R16, offsetof(struct pt_regs, r16));
	DEFINE(PT_R17, offsetof(struct pt_regs, r17));
	DEFINE(PT_R18, offsetof(struct pt_regs, r18));
	DEFINE(PT_R19, offsetof(struct pt_regs, r19));
	DEFINE(PT_R20, offsetof(struct pt_regs, r20));
	DEFINE(PT_R21, offsetof(struct pt_regs, r21));
	DEFINE(PT_R22, offsetof(struct pt_regs, r22));
	DEFINE(PT_R23, offsetof(struct pt_regs, r23));
	DEFINE(PT_R24, offsetof(struct pt_regs, r24));
	DEFINE(PT_R25, offsetof(struct pt_regs, r25));
	DEFINE(PT_GP, offsetof(struct pt_regs, gp));
	DEFINE(PT_FP, offsetof(struct pt_regs, fp));
	DEFINE(PT_SP, offsetof(struct pt_regs, sp));
	DEFINE(PT_RA, offsetof(struct pt_regs, ra));
	DEFINE(PT_EA, offsetof(struct pt_regs, ea));
	DEFINE(PT_BA, offsetof(struct pt_regs, ba));
	DEFINE(PT_MODE, offsetof(struct pt_regs, pt_mode));

	DEFINE(TI_TASK, offsetof(struct thread_info, task));
	DEFINE(TI_EXECDOMAIN, offsetof(struct thread_info, exec_domain));
	DEFINE(TI_FLAGS, offsetof(struct thread_info, flags));
	DEFINE(TI_CPU, offsetof(struct thread_info, cpu));
	DEFINE(TI_ADDR_LIMIT, offsetof(struct thread_info, addr_limit));
	DEFINE(_THREAD_SIZE, THREAD_SIZE);

	DEFINE(THREAD_KSP, offsetof(struct thread_struct, ksp));
	DEFINE(THREAD_USP, offsetof(struct thread_struct, usp));
	DEFINE(THREAD_WHICH_STACK, offsetof(struct thread_struct, which_stack));

	DEFINE(THREAD_SIZE, THREAD_SIZE);

	/* Offsets in thread_info structure */
	DEFINE(TI_TASK, offsetof(struct thread_info, task));
	DEFINE(TI_EXECDOMAIN, offsetof(struct thread_info, exec_domain));
	DEFINE(TI_FLAGS, offsetof(struct thread_info, flags));
	DEFINE(TI_CPU, offsetof(struct thread_info, cpu));

	return 0;
}
