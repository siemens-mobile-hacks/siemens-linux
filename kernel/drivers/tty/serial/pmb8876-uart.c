#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/clk.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/sysrq.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>

#include <linux/time.h>
#include <linux/timex.h>


/*
 *  Driver for PMB8876 serial ports
 *
 *  Copyright 2017 Siemens Narkomans (c)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>


#if defined(CONFIG_SERIAL_PMB8876_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/serial_core.h>


#define SERIAL_PMB8876_MAJOR			204
#define SERIAL_PMB8876_MINOR			16
#define SERIAL_PMB8876_DEVNAME			"ttyAM"
#define SERIAL_PMB8876_NR				1
#define PMB8876_UART_PORT_SIZE			0x80

#define PMB8876_USART0_BASE				0xf1000000
#define PMB8876_USART0_CLC(base)		(base)
#define PMB8876_USART0_BG(base)			(base + 0x14)
#define PMB8876_USART0_FDV(base)		(base + 0x18)
#define PMB8876_USART0_TXB(base)		(base + 0x20)
#define PMB8876_USART0_RXB(base)		(base + 0x24)
#define PMB8876_USART0_FCSTAT(base)		(base + 0x68)
#define PMB8876_USART0_ICR(base)		(base + 0x70)

#define PMB8876_CLOCK_RATE 				26000000


struct pmb8876_usrptr {
	int baudrate;
};


enum {
	UART_SPEED_57600 = 0x001901d8, 
	UART_SPEED_115200 = 0x000c01d8, 
	UART_SPEED_230400 = 0x000501b4, 
	UART_SPEED_460800 = 0x00000092, 
	UART_SPEED_614400 = 0x000000c3, 
	UART_SPEED_921600 = 0x00000127, 
	UART_SPEED_1228800 = 0x0000018a, 
	UART_SPEED_1600000 = 0x00000000, 
	UART_SPEED_1500000 = 0x000001d0
};






#define UART_PUT_CHAR(port, c) {							\
	int fc = 0;												\
	__raw_writeb(c, (void *)PMB8876_USART0_TXB(port->mapbase));	\
															\
	do {													\
		fc = __raw_readl((void *)PMB8876_USART0_FCSTAT(port->mapbase));	\
	} while( !(fc & 0x2 ) ); 								\
															\
	__raw_writel(fc | 0x2, (void *)PMB8876_USART0_ICR(port->mapbase));		\
}




static struct timer_list pmb8876_uart_tx_timer;
static struct timer_list pmb8876_uart_rx_timer;
static int rx_empty_cnt = 0;


static inline int pmb8876_baud_to_divisor(int speed)
{
	switch( speed ) {
		
		case 57600:
			return UART_SPEED_57600;
		
		case 115200:
			return UART_SPEED_115200;
			
		case 230400:
			return UART_SPEED_230400;
			
		case 460800:
			return UART_SPEED_460800;
			
		case 921600:
			return UART_SPEED_921600;
			
		case 1228800:
			return UART_SPEED_1228800;
			
		case 1600000:
			return UART_SPEED_1600000;
			
		case 1500000:
			return UART_SPEED_1500000;
	}
	
	return -1;
}


static inline int tx_enabled(struct uart_port *port)
{
	return port->unused[0] & 1;
}

static inline int rx_enabled(struct uart_port *port)
{
	return port->unused[0] & 2;
}

static inline int ms_enabled(struct uart_port *port)
{
	return port->unused[0] & 4;
}

static inline void ms_enable(struct uart_port *port, int enabled)
{
	if(enabled)
		port->unused[0] |= 4;
	else
		port->unused[0] &= ~4;
}

static inline void rx_enable(struct uart_port *port, int enabled)
{
	if(enabled)
		port->unused[0] |= 2;
	else
		port->unused[0] &= ~2;
}

static inline void tx_enable(struct uart_port *port, int enabled)
{
	if(enabled)
		port->unused[0] |= 1;
	else
		port->unused[0] &= ~1;
}


#ifdef SUPPORT_SYSRQ
static struct console pmb8876_console;
#endif

static void pmb8876uart_stop_tx(struct uart_port *port)
{
	if (tx_enabled(port)) {
		/* use disable_irq_nosync() and not disable_irq() to avoid self
		 * imposed deadlock by not waiting for irq handler to end,
		 * since this pmb8876uart_stop_tx() is called from interrupt context.
		 */
		//disable_irq_nosync(PMB8876_IRQ_UART_TX);
		tx_enable(port, 0);
	}
}

static void pmb8876uart_start_tx(struct uart_port *port)
{
	if (!tx_enabled(port)) {
		//enable_irq(PMB8876_IRQ_UART_TX);
		tx_enable(port, 1);
		mod_timer(&pmb8876_uart_tx_timer, jiffies + msecs_to_jiffies(1));
	}
}

static void pmb8876uart_stop_rx(struct uart_port *port)
{
	if (rx_enabled(port)) {
		//disable_irq(PMB8876_IRQ_UART_RX);
		rx_enable(port, 0);
	}
}

static void pmb8876uart_enable_ms(struct uart_port *port)
{
	if (!ms_enabled(port)) {
		//enable_irq(PMB8876_IRQ_UART_MODEM_STATUS);
		ms_enable(port,1);
	}
}


static unsigned int pmb8876uart_tx_empty(struct uart_port *port)
{
	int fc = __raw_readl((void *)PMB8876_USART0_FCSTAT(port->mapbase));
    return (fc & 0x2) == 0? TIOCSER_TEMT : 0;
}



static void pmb8876uart_rx_chars( unsigned long data )
{
	struct uart_port *port = (struct uart_port *)data;
	unsigned int ch, lsr, flg, max_count = 256, bsended = 0;

	while( max_count-- ) {
		
		int fc = __raw_readl((void *)PMB8876_USART0_FCSTAT(port->mapbase));
		if( !(fc & 0x4) )
			break;
		
		__raw_writel(fc | 0x4, (void *)PMB8876_USART0_ICR(port->mapbase));
		
		ch = __raw_readl((void *)PMB8876_USART0_RXB(port->mapbase)) & 0xff;
		flg = TTY_NORMAL;

		port->icount.rx++;
		bsended ++;

		if (uart_handle_sysrq_char(port, ch))
			goto ignore_char;

		uart_insert_char(port, lsr, 0, ch, flg);

ignore_char:;
	}
	
	if( bsended == 0 )
		rx_empty_cnt ++;
	
	else
		rx_empty_cnt = 0;
	
	tty_flip_buffer_push(&port->state->port);
	
	if( rx_enabled(port) ) {
		mod_timer(&pmb8876_uart_rx_timer, jiffies + msecs_to_jiffies(rx_empty_cnt < 100? 1 : 150));
		
		//if( rx_empty_cnt > 100 )
		//	pr_info("rx timer relax\n");
		
	} else
		mod_timer(&pmb8876_uart_rx_timer, jiffies + msecs_to_jiffies(150));
}


static void pmb8876uart_tx_chars( unsigned long data )
{
	//pr_info("tx work!\n");
	struct uart_port *port = (struct uart_port *)data;
	struct circ_buf *xmit = &port->state->xmit;
	unsigned int count;

	if (port->x_char) {
		UART_PUT_CHAR(port, port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		goto __ex;
	}

	if (uart_tx_stopped(port) || uart_circ_empty(xmit)) {
		pmb8876uart_stop_tx(port);
		goto __notx;
	}

	count = 16;	/* fifo size */
	while (!uart_circ_empty(xmit) && (count-- > 0)) {
		UART_PUT_CHAR(port, xmit->buf[xmit->tail]);

		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit)) {
		pmb8876uart_stop_tx(port);
		goto __notx;
	}

__ex:
	//mod_timer(&pmb8876_uart_tx_timer, jiffies + msecs_to_jiffies(10));
	pmb8876uart_tx_chars(data);
	
__notx:
	;
	
	//mod_timer(&pmb8876_uart_tx_timer, jiffies + msecs_to_jiffies(10));
}



static unsigned int pmb8876uart_get_mctrl(struct uart_port *port)
{
	unsigned int result = 0;
	/*unsigned int status;

	status = UART_GET_MSR(port);
	if (status & URMS_URDCD)
		result |= TIOCM_CAR;
	if (status & URMS_URDSR)
		result |= TIOCM_DSR;
	if (status & URMS_URCTS)
		result |= TIOCM_CTS;
	if (status & URMS_URRI)
		result |= TIOCM_RI;*/

	return result;
}

static void pmb8876uart_set_mctrl(struct uart_port *port, u_int mctrl)
{
	/*unsigned int mcr;

	mcr = UART_GET_MCR(port);
	if (mctrl & TIOCM_RTS)
		mcr |= URMC_URRTS;
	else
		mcr &= ~URMC_URRTS;

	if (mctrl & TIOCM_DTR)
		mcr |= URMC_URDTR;
	else
		mcr &= ~URMC_URDTR;

	UART_PUT_MCR(port, mcr);*/
}

static void pmb8876uart_break_ctl(struct uart_port *port, int break_state)
{
	/*unsigned int lcr;

	lcr = UART_GET_LCR(port);

	if (break_state == -1)
		lcr |= URLC_URSBC;
	else
		lcr &= ~URLC_URSBC;

	UART_PUT_LCR(port, lcr);*/
}


static int pmb8876uart_startup(struct uart_port *port)
{
	struct pmb8876_usrptr *usrptr;
	int retval = 0;

	//irq_modify_status(PMB8876_IRQ_UART_TX, IRQ_NOREQUEST, IRQ_NOAUTOEN);
	tx_enable(port, 0);
	rx_enable(port, 1);
	ms_enable(port, 1);
	
	usrptr = port->private_data = kmalloc(sizeof (struct pmb8876_usrptr), GFP_KERNEL);
	if( usrptr ) {
		usrptr->baudrate = -1;
	}
	
	setup_timer(&pmb8876_uart_tx_timer, pmb8876uart_tx_chars, (unsigned long)(port));
	setup_timer(&pmb8876_uart_rx_timer, pmb8876uart_rx_chars, (unsigned long)(port));
	
	//mod_timer(&pmb8876_uart_tx_timer, jiffies + msecs_to_jiffies(10));
	mod_timer(&pmb8876_uart_rx_timer, jiffies + msecs_to_jiffies(1));
	return retval;
}

static void pmb8876uart_shutdown(struct uart_port *port)
{
	/* disable break condition and fifos */
	struct pmb8876_usrptr *usrptr = port->private_data;
	if( usrptr ) kfree(usrptr);
}

static void pmb8876uart_set_termios(struct uart_port *port, struct ktermios *termios, struct ktermios *old)
{
	struct pmb8876_usrptr *usrptr = port->private_data;
	unsigned int baud, quot, new = 1;

	/*
	 * Ask the core to calculate the divisor for us.
	 */
	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/16);
	quot = uart_get_divisor(port, baud);

	
	if( usrptr && usrptr->baudrate == baud )
		new = 0;
	
	if( new ) {
		int bgfd = pmb8876_baud_to_divisor(baud);
		if( bgfd < 0 ) {
			pr_err("Unsupported baudrate %d\n", baud);
			return;
		}
			
		pr_info("ttyAM0: Setting baudrate %d\n", baud);
		__raw_writel( ((bgfd >> 16)), (void *)PMB8876_USART0_BG(port->mapbase) );
		__raw_writel( ((bgfd << 16) >> 16), (void *)PMB8876_USART0_FDV(port->mapbase) );
	}
	
	
	/*
	* Update the per-port timeout.
	*/
	uart_update_timeout(port, termios->c_cflag, baud);
	
	
	if( usrptr )
		usrptr->baudrate = baud;
}

static const char *pmb8876uart_type(struct uart_port *port)
{
	return port->type == PORT_PMB8876 ? "PMB8876" : NULL;
}

/*
 * Release the memory region(s) being used by 'port'
 */
static void pmb8876uart_release_port(struct uart_port *port)
{
	release_mem_region(port->mapbase, PMB8876_UART_PORT_SIZE);
}

/*
 * Request the memory region(s) being used by 'port'
 */
static int pmb8876uart_request_port(struct uart_port *port)
{
	return request_mem_region(port->mapbase, PMB8876_UART_PORT_SIZE,
			"serial_pmb8876") != NULL ? 0 : -EBUSY;
}

/*
 * Configure/autoconfigure the port.
 */
static void pmb8876uart_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_PMB8876;
		pmb8876uart_request_port(port);
	}
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int pmb8876uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;

	if (ser->type != PORT_UNKNOWN && ser->type != PORT_PMB8876)
		ret = -EINVAL;
	if (ser->irq != port->irq)
		ret = -EINVAL;
	if (ser->baud_base < 9600)
		ret = -EINVAL;
	return ret;
}


static struct uart_ops pmb8876uart_pops = {
	.tx_empty	= pmb8876uart_tx_empty,
	.set_mctrl	= pmb8876uart_set_mctrl,
	.get_mctrl	= pmb8876uart_get_mctrl,
	.stop_tx	= pmb8876uart_stop_tx,
	.start_tx	= pmb8876uart_start_tx,
	.stop_rx	= pmb8876uart_stop_rx,
	.enable_ms	= pmb8876uart_enable_ms,
	.break_ctl	= pmb8876uart_break_ctl,
	.startup	= pmb8876uart_startup,
	.shutdown	= pmb8876uart_shutdown,
	.set_termios	= pmb8876uart_set_termios,
	.type		= pmb8876uart_type,
	.release_port	= pmb8876uart_release_port,
	.request_port	= pmb8876uart_request_port,
	.config_port	= pmb8876uart_config_port,
	.verify_port	= pmb8876uart_verify_port,
};

static struct uart_port pmb8876uart_ports[SERIAL_PMB8876_NR] = {
	{
		.membase	= (void *)PMB8876_USART0_BASE,
		.mapbase	= PMB8876_USART0_BASE,
		.iotype		= SERIAL_IO_MEM,
		.irq		= -1,
		.uartclk	= PMB8876_CLOCK_RATE * 16,
		.fifosize	= 16,
		.ops		= &pmb8876uart_pops,
		.flags		= UPF_BOOT_AUTOCONF,
		.line		= 0,
	}
};

#ifdef CONFIG_SERIAL_PMB8876_CONSOLE
static void pmb8876_console_putchar(struct uart_port *port, int ch)
{
	UART_PUT_CHAR(port, ch);
}

static void pmb8876_console_write(struct console *co, const char *s, u_int count)
{
	struct uart_port *port = pmb8876uart_ports + co->index;

	uart_console_write(port, s, count, pmb8876_console_putchar);
}

static void __init pmb8876_console_get_options(struct uart_port *port, int *baud, int *parity, int *bits)
{
	struct pmb8876_usrptr *usrptr = port->private_data;
	
	*parity = 'n';
	*bits	= 8;
	
	if(usrptr)
		*baud = usrptr->baudrate;
	
	else
		*baud = 1600000;
}

static int __init pmb8876_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 1600000;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	port = uart_get_console(pmb8876uart_ports, SERIAL_PMB8876_NR, co);

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	else
		pmb8876_console_get_options(port, &baud, &parity, &bits);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver pmb8876_reg;

static struct console pmb8876_console = {
	.name		= SERIAL_PMB8876_DEVNAME,
	.write		= pmb8876_console_write,
	.device		= uart_console_device,
	.setup		= pmb8876_console_setup,
	.flags		= 0,
	.index		= -1,
	.data		= &pmb8876_reg,
};

static int __init pmb8876_console_init(void)
{
	add_preferred_console(SERIAL_PMB8876_DEVNAME, 0, NULL);
	register_console(&pmb8876_console);
	return 0;
}

console_initcall(pmb8876_console_init);

#define PMB8876_CONSOLE	&pmb8876_console
#else
#define PMB8876_CONSOLE	NULL
#endif

static struct uart_driver pmb8876_reg = {
	.owner			= THIS_MODULE,
	.driver_name	= "serial_pmb8876",
	.dev_name		= SERIAL_PMB8876_DEVNAME,
	.major			= SERIAL_PMB8876_MAJOR,
	.minor			= SERIAL_PMB8876_MINOR,
	.nr				= SERIAL_PMB8876_NR,
	.cons			= PMB8876_CONSOLE,
};

static int __init pmb8876uart_init(void)
{
	int i, ret;

	printk("Serial: PMB8876 UART driver\n");

	ret = uart_register_driver(&pmb8876_reg);
	if (ret)
		return ret;

	for (i = 0; i < SERIAL_PMB8876_NR; i++)
		uart_add_one_port(&pmb8876_reg, &pmb8876uart_ports[0]);

	return 0;
}

static void __exit pmb8876uart_exit(void)
{
	int i;

	for (i = 0; i < SERIAL_PMB8876_NR; i++)
		uart_remove_one_port(&pmb8876_reg, &pmb8876uart_ports[0]);
	uart_unregister_driver(&pmb8876_reg);
}

module_init(pmb8876uart_init);
module_exit(pmb8876uart_exit);

MODULE_DESCRIPTION("PMB8876 serial port driver");
MODULE_AUTHOR("Siemens Narkomans (c).");
MODULE_LICENSE("GPL");
