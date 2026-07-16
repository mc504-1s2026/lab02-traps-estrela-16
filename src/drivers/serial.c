#include <kernel/serial.h>
#include <kernel/mm.h>
#include <arch/io.h>
#include <arch/plic.h>
#include <arch/spinlock.h>
#include <arch/csr.h>

#define SERIAL_BUF_SIZE 256

/* Virtual address of the serial port via the direct map */
#define SERIAL_VA ((u64)SERIAL_BASE + KERNEL_DIRECT_MAP_START)

static struct {
	char buf[SERIAL_BUF_SIZE];
	size_t len;
	struct spinlock lock;
} dev;

static u8 serial_reg_read(u64 reg)
{
	return ioread8((void *)(SERIAL_VA + reg));
}

static void serial_reg_write(u64 reg, u8 val)
{
	iowrite8(val, (void *)(SERIAL_VA + reg));
}

void serial_init()
{
	spin_init(&dev.lock);
	dev.len = 0;

	/* Disable all UART interrupts */
	serial_reg_write(SERIAL_IER, 0x00);

	/* Enable FIFO, clear RX and TX FIFOs */
	serial_reg_write(SERIAL_FCR,
		SERIAL_FCR_FIFO_ENABLE |
		SERIAL_FCR_RX_FIFO_CLEAR |
		SERIAL_FCR_TX_FIFO_CLEAR);

	/* Set word length to 8 bits (LCR bits 0:1 = 0b11) */
	serial_reg_write(SERIAL_LCR, 0x03);
}

void serial_irq_enable()
{
	/* Enable the received data available interrupt in the UART */
	serial_reg_write(SERIAL_IER, SERIAL_IER_ERBFI);

	/* Configure the PLIC for the serial interrupt */
	plic_irq_set_priority(IRQ_SERIAL, 1);
	plic_hart_set_threshold(0, 0);
	plic_hart_enable_irq(0, IRQ_SERIAL);

	/* Enable external interrupts in the sie CSR */
	csr_set(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq_disable()
{
	serial_reg_write(SERIAL_IER, 0x00);
	csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq()
{
	spin_lock(&dev.lock);

	/* Read all available bytes from the UART FIFO */
	/* Read all available bytes from the UART FIFO */
	/* 0x01 is the Data Ready (DR) bit in the LSR */
	while (serial_reg_read(SERIAL_LSR) & 0x01) {
		u8 c = serial_reg_read(SERIAL_RBR);
		if (dev.len < SERIAL_BUF_SIZE) {
			dev.buf[dev.len++] = (char)c;
		}
	}

	spin_unlock(&dev.lock);
}

size_t serial_read(char *buf)
{
	u64 flags = spin_lock_irqsave(&dev.lock);
	size_t n = dev.len;
	for (size_t i = 0; i < n; i++) {
		buf[i] = dev.buf[i];
	}
	dev.len = 0;
	spin_unlock_irqrestore(&dev.lock, flags);
	return n;
}

void serial_putc(char c)
{
	/* Wait for the transmitter holding register to be empty */
	while (!(serial_reg_read(SERIAL_LSR) & SERIAL_LSR_THRE))
		;
	serial_reg_write(SERIAL_THR, (u8)c);
}

void serial_puts(char *str)
{
	while (*str) {
		serial_putc(*str);
		str++;
	}
}
