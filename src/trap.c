#include <kernel/trap.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <kernel/serial.h>
#include <arch/csr.h>
#include <arch/plic.h>
#include <arch/timer.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

static void handle_irq(u64 scause)
{
	if (scause == TRAP_TIMER_IRQ) {
		timer_irq();
	} else if (scause == TRAP_EXTERNAL_IRQ) {
		u32 irq = plic_hart_claim_irq(0);
		if (irq == IRQ_SERIAL) {
			serial_irq();
		}
		if (irq != 0) {
			plic_hart_complete_irq(0, irq);
		}
	}
}

static void handle_exception(u64 scause)
{
	u64 stval = csr_read(CSR_STVAL);
	u64 sepc = csr_read(CSR_SEPC);

	switch (scause) {
	case EXCEPTION_INST_ACCESS_FAULT:
		panic("instruction access fault at sepc=%p, stval=%p\n",
		      sepc, stval);
		break;
	case EXCEPTION_LOAD_ACCESS_FAULT:
		panic("load access fault at sepc=%p, stval=%p\n",
		      sepc, stval);
		break;
	case EXCEPTION_STORE_ACCESS_FAULT:
		panic("store access fault at sepc=%p, stval=%p\n",
		      sepc, stval);
		break;
	case EXCEPTION_INST_PAGE_FAULT:
		panic("instruction page fault at sepc=%p, stval=%p\n",
		      sepc, stval);
		break;
	case EXCEPTION_LOAD_PAGE_FAULT:
		panic("load page fault at sepc=%p, stval=%p\n",
		      sepc, stval);
		break;
	case EXCEPTION_STORE_PAGE_FAULT:
		panic("store page fault at sepc=%p, stval=%p\n",
		      sepc, stval);
		break;
	default:
		panic("unhandled exception: scause=%p, sepc=%p, stval=%p\n",
		      scause, sepc, stval);
		break;
	}
}

void trap_setup()
{
	/* Write the trap handler address to stvec */
	csr_write(CSR_STVEC, (u64)trap_entry);
}

void handle_trap()
{
	u64 scause = csr_read(CSR_SCAUSE);

	if (scause & TRAP_IRQ_BIT) {
		handle_irq(scause);
	} else {
		handle_exception(scause);
	}
}

void hart_irq_enable()
{
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

u64 hart_irq_save()
{
	return csr_read_clear(CSR_SSTATUS, CSR_SSTATUS_SIE) & CSR_SSTATUS_SIE;
}

void hart_irq_restore(u64 flags)
{
	if (flags & CSR_SSTATUS_SIE) {
		csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
	}
}

void hart_irq_disable()
{
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}
