#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>

static struct Taskstate ts;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames)/sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	return "(unknown trap)";
}


void
idt_init(void)
{
	extern struct Segdesc gdt[];
	
	// LAB 3: Your code here.
    extern void trap_handle_divide_error();
    extern void trap_handle_debug_exception();
    extern void trap_handle_non_maskable_interrupt();
    extern void trap_handle_breakpoint();
    extern void trap_handle_overflow();
    extern void trap_handle_bounds_check();
    extern void trap_handle_illegal_opcode();
    extern void trap_handle_device_not_available();
    extern void trap_handle_double_fault();
    extern void trap_handle_invalid_task_switch_segment();
    extern void trap_handle_segment_not_present();
    extern void trap_handle_stack_exception();
    extern void trap_handle_general_protection_fault();
    extern void trap_handle_page_fault();
    extern void trap_handle_floating_point_error();
    extern void trap_handle_aligment_check();
    extern void trap_handle_machine_check();
    extern void trap_handle_simd_floating_point_error();
    extern void trap_handle_system_call();

    SETGATE(idt[T_DIVIDE], 0, GD_KT, trap_handle_divide_error, 0);
    SETGATE(idt[T_DEBUG], 0, GD_KT, trap_handle_debug_exception, 0);
    SETGATE(idt[T_NMI], 0, GD_KT, trap_handle_non_maskable_interrupt, 0);
    SETGATE(idt[T_BRKPT], 0, GD_KT, trap_handle_breakpoint, 3);
    SETGATE(idt[T_OFLOW], 0, GD_KT, trap_handle_overflow, 0);
    SETGATE(idt[T_BOUND], 0, GD_KT, trap_handle_bounds_check, 0);
    SETGATE(idt[T_ILLOP], 0, GD_KT, trap_handle_illegal_opcode, 0);
    SETGATE(idt[T_DEVICE], 0, GD_KT, trap_handle_device_not_available, 0);
    SETGATE(idt[T_DBLFLT], 0, GD_KT, trap_handle_double_fault, 0);
    SETGATE(idt[T_TSS], 0, GD_KT, trap_handle_invalid_task_switch_segment, 0);
    SETGATE(idt[T_SEGNP], 0, GD_KT, trap_handle_segment_not_present, 0);
    SETGATE(idt[T_STACK], 0, GD_KT, trap_handle_stack_exception, 0);
    SETGATE(idt[T_GPFLT], 0, GD_KT, trap_handle_general_protection_fault, 0);
    SETGATE(idt[T_PGFLT], 0, GD_KT, trap_handle_page_fault, 0);
    SETGATE(idt[T_FPERR], 0, GD_KT, trap_handle_floating_point_error, 0);
    SETGATE(idt[T_ALIGN], 0, GD_KT, trap_handle_aligment_check, 0);
    SETGATE(idt[T_MCHK], 0, GD_KT, trap_handle_machine_check, 0);
    SETGATE(idt[T_SIMDERR], 0, GD_KT, trap_handle_simd_floating_point_error, 0);
    SETGATE(idt[T_SYSCALL], 0, GD_KT, trap_handle_system_call, 3);
	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	ts.ts_esp0 = KSTACKTOP;
	ts.ts_ss0 = GD_KD;

	// Initialize the TSS field of the gdt.
	gdt[GD_TSS >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
					sizeof(struct Taskstate), 0);
	gdt[GD_TSS >> 3].sd_s = 0;

	// Load the TSS
	ltr(GD_TSS);

	// Load the IDT
	asm volatile("lidt idt_pd");
}

void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	cprintf("  err  0x%08x\n", tf->tf_err);
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	cprintf("  esp  0x%08x\n", tf->tf_esp);
	cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

static void
trap_dispatch(struct Trapframe *tf)
{
	// Handle processor exceptions.
	// LAB 3: Your code here.
	if (tf->tf_trapno == T_PGFLT) {
		page_fault_handler(tf);
		return;
	} else if (tf->tf_trapno == T_SYSCALL) {
		int32_t ret = syscall(tf->tf_regs.reg_eax, tf->tf_regs.reg_edx, tf->tf_regs.reg_ecx,
							  tf->tf_regs.reg_ebx, tf->tf_regs.reg_edi, tf->tf_regs.reg_esi);
		tf->tf_regs.reg_eax = ret;
		return;
	}

	// Unexpected trap: The user process or the kernel has a bug.
	print_trapframe(tf);
	if (tf->tf_cs == GD_KT)
		panic("unhandled trap in kernel");
	else {
		env_destroy(curenv);
		return;
	}
}

void
trap(struct Trapframe *tf)
{
	cprintf("Incoming TRAP frame at %p\n", tf);

	if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		assert(curenv);
		curenv->env_tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}
	
	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	// Return to the current environment, which should be runnable.
	assert(curenv && curenv->env_status == ENV_RUNNABLE);
	env_run(curenv);
}


void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

	// Handle kernel-mode page faults.
	
	// LAB 3: Your code here.
    if ((tf->tf_cs & 3) != 3)
        panic("got page fault in kernel");

    // We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

	// Destroy the environment that caused the fault.
	cprintf("[%08x] user fault va %08x ip %08x\n",
		curenv->env_id, fault_va, tf->tf_eip);
	print_trapframe(tf);
	env_destroy(curenv);
}

