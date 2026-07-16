#include <arch/timer.h>
#include <arch/csr.h>

volatile int alarm_pending = 0;

u64 timer_read()
{
	return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
	/* Set stimecmp to max value to prevent immediate fire */
	csr_write(CSR_STIMECMP, ~0UL);
	/* Enable the timer interrupt source in sie */
	csr_set(CSR_SIE, CSR_SIE_STIE);
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	u64 target = timer_read() + secs * TIMER_FREQ;
	csr_write(CSR_STIMECMP, target);
}

void timer_irq()
{
	alarm_pending = 1;
	/* Prevent repeated fires until the next alarm is set */
	csr_write(CSR_STIMECMP, ~0UL);
}
