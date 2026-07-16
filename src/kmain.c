#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

extern int _hartid[];

/* defined in timer.c */
extern volatile int alarm_pending;

static void process_command(char *cmd)
{
	if (strcmp(cmd, "uptime") == 0) {
		u64 secs = timer_read() / TIMER_FREQ;
		char buf[32];
		snprintf(buf, sizeof(buf), "%lus\n", secs);
		serial_puts(buf);
	} else if (strncmp(cmd, "echo ", 5) == 0) {
		serial_puts(cmd + 5);
		serial_putc('\n');
	} else if (strncmp(cmd, "alarm ", 6) == 0) {
		u64 secs = strtou64(cmd + 6, 10);
		timer_set_alarm(secs);
	}
}

void kmain()
{
	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);
	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();
	info("enabling timer...\n");
	timer_irq_enable();
	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();

	/* Enable global interrupts now that everything is configured */
	hart_irq_enable();

	char cmd_buf[256];
	size_t cmd_len = 0;
	char input[256];

	serial_puts("> ");

	while (1) {
		/* Check if a timer alarm has fired */
		if (alarm_pending) {
			alarm_pending = 0;
			serial_puts("alarm\n");
		}

		/* Read any available serial data */
		size_t n = serial_read(input);
		for (size_t i = 0; i < n; i++) {
			char c = input[i];
			if (c == '\r') {
				serial_puts("\r\n");
				cmd_buf[cmd_len] = '\0';
				process_command(cmd_buf);
				cmd_len = 0;
				serial_puts("> ");
			} else if (c == '\n') {
				/* Ignore LF (some terminals send CR+LF) */
			} else if (c == 127 || c == '\b') {
				if (cmd_len > 0) {
					cmd_len--;
					serial_puts("\b \b");
				}
			} else {
				if (cmd_len < sizeof(cmd_buf) - 1) {
					cmd_buf[cmd_len++] = c;
					serial_putc(c);
				}
			}
		}
	}
}
