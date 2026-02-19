# Debugging Preemption Issue

## Goal Description
The goal is to fix the issue where the scheduler fails to preempt the running task (specifically PID 0), causing other tasks (PID 1, 2, 3...) to starve. This is likely due to the timer interrupt not correctly triggering a context switch or interrupts being masked in EL0.

## User Review Required
> [!IMPORTANT]
> This plan involves modifying low-level kernel code including interrupt handlers and context switching logic.

## Proposed Changes
### Interrupt Handling & Timer
#### [MODIFY] [src/kernel/timer.c](file:///wsl.localhost/Ubuntu-24.04/home/frank0988/workspace/osc/lab5/src/kernel/timer.c)
- Verify `core_timer_enable` sets `cntkctl_el1` correctly to allow EL0 access to virtual counter (bit 0 and 1).
- Check if timer interrupt handler calls `schedule()`.

#### [MODIFY] [src/kernel/trap/irq.c](file:///wsl.localhost/Ubuntu-24.04/home/frank0988/workspace/osc/lab5/src/kernel/trap/irq.c) (Path to be confirmed - likely exception.c)
- Confirm the IRQ handler dispatch logic correctly identifies the core timer interrupt.

### Context Switch
#### [MODIFY] [src/kernel/kernel_start.S](file:///wsl.localhost/Ubuntu-24.04/home/frank0988/workspace/osc/lab5/src/kernel/kernel_start.S)
- Check `from_el1_to_el0` to ensure `SPSR_EL1` has DAIF bits cleared (0) to enable interrupts in EL0.

### Scheduler
#### [MODIFY] [src/kernel/sche/sched.c](file:///wsl.localhost/Ubuntu-24.04/home/frank0988/workspace/osc/lab5/src/kernel/sche/sched.c)
- Ensure `schedule()` handles the case where `pid0` is preempted.

## Verification Plan
### Automated Tests
- Run `run forktest` (using the shell or `task.md` instruction).
- Verify that multiple processes run concurrently and `pid0` does not block others.
- Check UART output for interleaving of process outputs.

### Manual Verification
1.  Build the kernel using `make`.
2.  Run with QEMU: `make run`.
3.  Execute `run forktest`.
4.  Observe if strict priority enforcement (PID 0 always running) is broken by time slicing.
