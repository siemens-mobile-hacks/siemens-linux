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

#include <mach/irqs.h>


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
#define SERIAL_PMB8876_NR			2
#define PMB8876_UART_PORT_SIZE			0x80
#define PMB8876_UART_FIFO_SIZE			8


/* REGS */
#define PMB8876_USART0_BASE			0xf1000000
#define PMB8876_USART1_BASE			0xf1800000
#define PMB8876_USART_CLC(base)			(base)
#define PMB8876_USART_PISEL(base)		(base + 0x04)
#define PMB8876_USART_ID(base)			(base + 0x08)
#define PMB8876_USART_CON(base)			(base + 0x10)
#define PMB8876_USART_BG(base)			(base + 0x14)
#define PMB8876_USART_FDV(base)			(base + 0x18)
#define PMB8876_USART_PMW(base)			(base + 0x1C)
#define PMB8876_USART_TXB(base)			(base + 0x20)
#define PMB8876_USART_RXB(base)			(base + 0x24)
#define PMB8876_USART_ABCON(base)		(base + 0x30)
#define PMB8876_USART_ABSTAT(base)		(base + 0x34)
#define PMB8876_USART_RXFCON(base)		(base + 0x40)
#define PMB8876_USART_TXFCON(base)		(base + 0x44)
#define PMB8876_USART_FSTAT(base)		(base + 0x48)
#define PMB8876_USART_WHBCON(base)		(base + 0x50)
#define PMB8876_USART_FCCON(base)		(base + 0x5C)	/* Flowcontrol control register */
#define PMB8876_USART_IMSC(base)		(base + 0x64)
#define PMB8876_USART_FCSTAT(base)		(base + 0x68)
#define PMB8876_USART_ICR(base)			(base + 0x70)
#define PMB8876_USART_ISR(base)			(base + 0x74)

#define PMB8876_CLOCK_RATE 			26000000

/* BITS */
#define CLC_SMC_CLK_DIV(x)	((x << 16) & 0xFF0000)
#define CLC_RMC_CLK_DIV(x)	((x <<  8) & 0x00FF00)
#define CLC_FSOE		(1 << 5)	/* Fast shut off enable (1: enable; 0: disable) */
#define CLC_SBWE		(1 << 4)	/* Suspend bit write enable (1: enable; 0: disable) */
#define CLC_EDIS		(1 << 3)	/* External request disable (1: disable; 0: enable) */
#define CLC_SPEN		(1 << 2)	/* Suspend bit enable (1: enable; 0: disable) */
#define CLC_DISS		(1 << 1)	/* Disable status bit (1: disable; 0: enable) */
#define CLC_DISR		(1 << 0)	/* Disable request bit (1: enable; 0: disable) */

#define	CON_R			(1 << 15)	/* Baud rate generator run control (0: disable; 1: enable) */
#define CON_LB			(1 << 14)	/* Loopback mode (0: disable; 1: enable) */
#define CON_BRS			(1 << 13)	/* Baudrate selection (0: Pre-scaler /2; 1: Pre-scaler / 3) */
#define CON_ODD			(1 << 12)	/* Parity selection (0: even; 1: odd)  */
#define	CON_FDE			(1 << 11)	/* Fraction divider enable (0: disable; 1: enable) */
#define CON_OE			(1 << 10)	/* Overrun error flag */
#define CON_FE			(1 <<  9)	/* Framing error flag */
#define CON_PE			(1 <<  8)	/* Parity error flag */
#define CON_OEN			(1 <<  7)	/* Overrun check enable (0: ignore; 1: check) */
#define CON_FEN			(1 <<  6)	/* Framing error check (0: ignore; 1: check) */
#define CON_PEN			(1 <<  5)	/* Parity check enable (0: ignore; 1: check) */
#define CON_REN			(1 <<  4)	/* Receiver bit enable (0: disable; 1: enable) */
#define CON_STP			(1 <<  3)	/* Number of stop bits (0: 1 stop bit; 1: two stop bits) */
#define CON_MODE_MASK		(7)		/* Mask for mode control */

#define WHBCON_SETOE		(1 << 13)	/* Set overrun error flag */
#define WHBCON_SETFE		(1 << 12)	/* Set framing error flag */
#define WHBCON_SETPE		(1 << 11)	/* Set parity error flag */
#define WHBCON_CLROE		(1 << 10)	/* Clear overrun error flag */
#define WHBCON_CLRFE		(1 <<  9)	/* Clear framing error flag */
#define WHBCON_CLRPE		(1 <<  8)	/* Clear parity error flag */
#define WHBCON_SETREN		(1 <<  5)	/* Set receiver enable bit */
#define WHBCON_CLRREN		(1 <<  4)	/* Clear receiver enable bit */

#define RX_DMA_ENABLE		(1 <<  1)	/* Receive DMA enable (0: disable, 1: enable) */
#define TX_DMA_ENABLE		(1 <<  0)	/* Transmit DMA enable (0: disable, 1: enable) */

#define ISR_TMO			(1 <<  7)	/* RX timeout interrupt mask */
#define ISR_CTS			(1 <<  6)	/* CTS interrupt mask */
#define ISR_ABDET		(1 <<  5)	/* Autobaud detected interrupt mask */
#define ISR_ABSTART		(1 <<  4)	/* Autobaud start interrupt mask */
#define ISR_ERR			(1 <<  3)	/* Error interrupt mask */
#define ISR_RX			(1 <<  2)	/* Receive interrupt mask */
#define ISR_TB			(1 <<  1)	/* Transmit buffer interrupt mask */
#define ISR_TX			(1 <<  0)	/* Transmit interrupt mask */

#define ICR_TMO			(1 <<  7)	/* RX timeout interrupt mask */
#define ICR_CTS			(1 <<  6)	/* CTS interrupt mask */
#define ICR_ABDET		(1 <<  5)	/* Autobaud detected interrupt mask */
#define ICR_ABSTART		(1 <<  4)	/* Autobaud start interrupt mask */
#define ICR_ERR			(1 <<  3)	/* Error interrupt mask */
#define ICR_RX			(1 <<  2)	/* Receive interrupt mask */
#define ICR_TB			(1 <<  1)	/* Transmit buffer interrupt mask */
#define ICR_TX			(1 <<  0)	/* Transmit interrupt mask */

#define FCSTAT_RTS		(1 <<  1)	/* RTS Status (0: inactive; 1: active) */
#define FCSTAT_CTS		(1 <<  0)	/* CTS Status (0: inactive; 1: active) */
#define FCCON_RTS_TRIGGER(x)	((x << 8) & 0x3F00) /* RTS receive FIFO trigger level */
#define FCCON_RTS		(1 <<  4)	/* RTS control bit */
#define FCCON_CTSEN		(1 <<  1)	/* CTS enable (0: disable; 1: enable) */
#define FCCON_RTSEN		(1 <<  0)	/* RTS enbled (0: disable; 1: enable) */

#define ABCON_RXINV		(1 << 11)	/* Receive invert enable (0: disable; 1: enable) */
#define ABCON_TXINV		(1 << 10)	/* Transmit invert enable (0: disable; 1: enable) */
#define ABCON_ABEM_ECHO_DET	(1 <<  8)	/* Autobaud echo mode enabled during detection */
#define ABCON_ABEM_ECHO_ALWAYS	(1 <<  9)	/* Autobaud echo mode always enabled */
#define ABCON_FCDETEN		(1 <<  4)	/* Fir char of two byte frame detect */
#define ABCON_ABDETEN		(1 <<  3)	/* Autobaud detection interrupt enable (0: dis; 1: en) */
#define ABCON_ABSTEN		(1 <<  2)	/* Start of autobaud detect interrupt (0: dis; 1: en) */
#define ABCON_AUREN		(1 <<  1)	/* Auto control of CON.REN (too complex for here) */
#define ABCON_ABEN		(1 <<  0)	/* Autobaud detection enable */

#define ABSTAT_DETWAIT		(1 <<  4)	/* Autobaud detect is waiting */
#define ABSTAT_SCCDET		(1 <<  3)	/* Second character with capital letter detected */
#define ABSTAT_SCSDET		(1 <<  2)	/* Second character with small letter detected */
#define ABSTAT_FCCDET		(1 <<  1)	/* First character with capital letter detected */
#define ABSTAT_FCSDET		(1 <<  0)	/* First character with small letter detected */

#define RXFCON_RXFITL(x)	((x & 8) <<  8)	/* Receive FIFO interrupt trigger level */
#define RXFCON_RXTMEN		(1 <<  2)	/* Receive FIFO transparent mode enable */
#define RXFCON_RXFFLU		(1 <<  1)	/* Receive FIFO flush */
#define RXFCON_RXFEN		(1 <<  0)	/* Receive FIFO enable */

#define TXFCON_TXFITL(x)	((x & 8) <<  8)	/* Transmit FIFO interrupt trigger level */
#define TXFCON_TXTMEN		(1 <<  2)	/* Transmit FIFO transparent mode enable */
#define TXFCON_TXFFLU		(1 <<  1)	/* Transmit FIFO flush */
#define TXFCON_TXFEN		(1 <<  0)	/* Transmit FIFO enable */

#define FSTAT_TXFFL		(0xF <<  8)	/* Transmit FIFO filling level mask */
#define FSTAT_RXFFL		(0xF)		/* Receive FIFO filling level mask */



/*
 * UART IO
 */

#define UART_PUT_CHAR(port, c)		__raw_writel(c, (void *)PMB8876_USART_TXB(port->mapbase))
#define UART_GET_CHAR(port)			__raw_readl((void *)PMB8876_USART_RXB(port->mapbase))
#define PMB8876_CLR_TX_INT(port)	__raw_writel(ICR_TX, (void *)PMB8876_USART_ICR(port->mapbase))
#define PMB8876_CLR_RX_INT(port)	__raw_writel(ICR_RX, (void *)PMB8876_USART_ICR(port->mapbase))
#define UART_FSTAT(port)			__raw_readl((void *)PMB8876_USART_FSTAT(port->mapbase))
#define UART_FCSTAT(port)			__raw_readl((void *)PMB8876_USART_FCSTAT(port->mapbase))
#define UART_CON(port)				__raw_readl((void *)PMB8876_USART_CON(port->mapbase))

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



struct pmb8876_usrptr {
	int baudrate;
};





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
	u32 irq_base = port->irq;
	if (tx_enabled(port)) {
		/* use disable_irq_nosync() and not disable_irq() to avoid self
		 * imposed deadlock by not waiting for irq handler to end,
		 * since this pmb8876uart_stop_tx() is called from interrupt context.
		 */
		disable_irq_nosync(irq_base + PMB8876_UART_TX_IRQ_OFF);
		tx_enable(port, 0);
	}
}

static void pmb8876uart_start_tx(struct uart_port *port)
{
	u32 irq_base = port->irq;
	if (!tx_enabled(port)) {
		enable_irq(irq_base + PMB8876_UART_TX_IRQ_OFF);
		tx_enable(port, 1);
	}
}

static void pmb8876uart_stop_rx(struct uart_port *port)
{
	u32 irq_base = port->irq;
	if (rx_enabled(port)) {
		disable_irq(irq_base + PMB8876_UART_RX_IRQ_OFF);
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
	int fc = UART_FCSTAT(port);
	return (fc & ISR_TB) == 0? TIOCSER_TEMT : 0;
}



static irqreturn_t pmb8876uart_linest_handler(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	unsigned int lsr, tmp, ch = 1;
	
	spin_lock(&port->lock);
	__raw_writel(ICR_ERR, (void *)PMB8876_USART_ICR(port->mapbase));
	
	lsr = UART_CON(port);
	if( lsr & CON_OE ) {
		tmp = __raw_readl((void *)PMB8876_USART_WHBCON(port->mapbase));
		__raw_writel(tmp | WHBCON_CLROE, (void *)PMB8876_USART_WHBCON(port->mapbase));
		ch = 0;
		port->icount.overrun++;
	}
	
	if( lsr & CON_FE ) {
		tmp = __raw_readl((void *)PMB8876_USART_WHBCON(port->mapbase));
		__raw_writel(tmp | WHBCON_CLRFE, (void *)PMB8876_USART_WHBCON(port->mapbase));
		ch = 0;
		port->icount.frame++;
	}
	
	if( !ch )
		uart_insert_char(port, lsr, CON_OE, ch, TTY_NORMAL);
	
	spin_unlock(&port->lock);
	return IRQ_HANDLED;
}



static irqreturn_t pmb8876uart_rx_chars(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	unsigned int status, ch, lsr = 0, flg, tmp;

	spin_lock(&port->lock);
	
	/* reset the RX flag interrupt */
	__raw_writel(ICR_RX | ICR_ERR, (void *)PMB8876_USART_ICR(port->mapbase));
	
	if( port->line == 1 )
	    pr_info("RX UART1 work!\n");
	
	while( ((status = UART_FSTAT(port)) & FSTAT_RXFFL) )
	{
		flg = TTY_NORMAL;
		
		// Get char
		ch = UART_GET_CHAR(port) & 0xff;
		port->icount.rx++;
		
		lsr = UART_CON(port);
		if( lsr & CON_OE ) {
			tmp = __raw_readl((void *)PMB8876_USART_WHBCON(port->mapbase));
			__raw_writel(tmp | WHBCON_CLROE, (void *)PMB8876_USART_WHBCON(port->mapbase));
			port->icount.overrun++;
		}
		
		if( lsr & CON_FE ) {
			tmp = __raw_readl((void *)PMB8876_USART_WHBCON(port->mapbase));
			__raw_writel(tmp | WHBCON_CLRFE, (void *)PMB8876_USART_WHBCON(port->mapbase));
			port->icount.frame++;
		}

		if (uart_handle_sysrq_char(port, ch))
			goto ignore_char;

		uart_insert_char(port, lsr, CON_OE, ch, flg);
		//tty_insert_flip_char(&port->state->port, ch, flg);
		
ignore_char:;
	}
	
	tty_flip_buffer_push(&port->state->port);
	
	if( (UART_FSTAT(port) & FSTAT_RXFFL) )
		__raw_writel(ISR_RX, (void *)PMB8876_USART_ISR(port->mapbase));
	
	spin_unlock(&port->lock);
	
	return IRQ_HANDLED;
}



static irqreturn_t pmb8876uart_tx_chars(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	struct circ_buf *xmit = &port->state->xmit;
	
	if( port->line == 1 )
	    pr_info("TX UART1 work!\n");

	spin_lock(&port->lock);
	
	if( port->x_char ) {
		UART_PUT_CHAR(port, port->x_char);

		port->icount.tx++;
		port->x_char = 0;
		spin_unlock(&port->lock);
		return IRQ_HANDLED;
	}

	if( uart_tx_stopped(port) || uart_circ_empty(xmit) ) {
		pmb8876uart_stop_tx(port);
		spin_unlock(&port->lock);
		return IRQ_HANDLED;
	}
	
	
	while( !uart_circ_empty(xmit) && (UART_FSTAT(port) & FSTAT_TXFFL) < PMB8876_UART_FIFO_SIZE ) {
		UART_PUT_CHAR(port, xmit->buf[xmit->tail]);

		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}
	
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		pmb8876uart_stop_tx(port);

	spin_unlock(&port->lock);
	return IRQ_HANDLED;
}


static unsigned int pmb8876uart_get_mctrl(struct uart_port *port)
{
	unsigned int result = 0;
	unsigned int status;

	status = readl((void *)PMB8876_USART_FCSTAT(port->mapbase));
	if (status & FCSTAT_CTS)
		result |= TIOCM_CTS;
	if (status & FCSTAT_RTS)
		result |= TIOCM_RTS;

	return result;
}

static void pmb8876uart_set_mctrl(struct uart_port *port, u_int mctrl)
{
	unsigned int mcr;

	mcr = readl((void *)PMB8876_USART_FCCON(port->mapbase));
	if (mctrl & TIOCM_CTS)
		mcr |= FCCON_CTSEN;
	else
		mcr &= ~FCCON_CTSEN;

	if (mctrl & TIOCM_RTS)
		mcr |= FCCON_RTSEN;
	else
		mcr &= ~FCCON_RTSEN;

	//writel(mcr, (void *)PMB8876_USART_FCCON(port->mapbase));
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
	static const char uart0_tx[] = "UART0 TX";
	static const char uart0_rx[] = "UART0 RX";
	static const char uart0_ls[] = "UART0 LineStatus";
	
	static const char uart1_tx[] = "UART1 TX";
	static const char uart1_rx[] = "UART1 RX";
	static const char uart1_ls[] = "UART1 LineStatus";
	
	const char *uart_tx, *uart_rx, *uart_ls;
	
	struct pmb8876_usrptr *usrptr;
	int retval = 0;
	unsigned int conr = 0;
	
	u32 irq_base = port->irq;

	/* setup irq hardware priority */
	pmb8876_set_irq_priority(irq_base + PMB8876_UART_TX_IRQ_OFF, 0xa);
	pmb8876_set_irq_priority(irq_base + PMB8876_UART_RX_IRQ_OFF, 0xa);
	pmb8876_set_irq_priority(irq_base + PMB8876_UART_LS_IRQ_OFF, 0x1);
	
	/* change type of tx irq */
	irq_modify_status(irq_base + PMB8876_UART_TX_IRQ_OFF, IRQ_NOREQUEST, IRQ_NOAUTOEN);
	
	/* reset */
	tx_enable(port, 0);
	rx_enable(port, 1);
	ms_enable(port, 1);

	if( irq_base == PMB8876_UART0_TX_IRQ ) {
	    uart_tx = (const char *)uart0_tx;
	    uart_rx = (const char *)uart0_rx;
	    uart_ls = (const char *)uart0_ls;
	} else {
	    uart_tx = (const char *)uart1_tx;
	    uart_rx = (const char *)uart1_rx;
	    uart_ls = (const char *)uart1_ls;
	}
	
	pr_info("Register UART%d irqs: TX %d, RX %d, LS %d\n", port->line,
	    irq_base + PMB8876_UART_TX_IRQ_OFF,
	    irq_base + PMB8876_UART_RX_IRQ_OFF,
	    irq_base + PMB8876_UART_LS_IRQ_OFF
	);
	
	// fixme
	usrptr = port->private_data = kmalloc(sizeof (struct pmb8876_usrptr), GFP_KERNEL);
	if( usrptr ) usrptr->baudrate = -1;
	
	__raw_writel((1 << 8) | 8, (void *)PMB8876_USART_CLC(port->mapbase));
	
	pr_info("UART%d clc: %X\n", port->line, readl((void *)PMB8876_USART_CLC(port->mapbase)));
	
	/* set async mode */
	conr = (__raw_readl( (void *)PMB8876_USART_CON(port->mapbase) ));
	__raw_writel( (conr & (~CON_MODE_MASK)) | 1 | CON_OEN | CON_FEN, (void *)PMB8876_USART_CON(port->mapbase) );
	
	/* enable RX/TX fifo's with interrupt at first byte placed in buffer */
	__raw_writel( (RXFCON_RXFEN | RXFCON_RXFITL(1)), (void *)PMB8876_USART_RXFCON(port->mapbase) );
	__raw_writel( (TXFCON_TXFEN | TXFCON_TXFITL(1)), (void *)PMB8876_USART_TXFCON(port->mapbase) );
	
	/* unmask rx/tx/err interrupt */
	__raw_writel( (ISR_RX | ISR_TX | ISR_ERR), (void *)PMB8876_USART_IMSC(port->mapbase) );
	
	
	/*
	 * Allocate the IRQ
	 */
	retval = request_irq(irq_base + PMB8876_UART_TX_IRQ_OFF, pmb8876uart_tx_chars, 0, uart_tx, port);
	if (retval) {
		pr_err("Failed to request TX irq: %d\n", retval);
		goto err_tx;
	}
	
	retval = request_irq(irq_base + PMB8876_UART_RX_IRQ_OFF, pmb8876uart_rx_chars, 0, uart_rx, port);
	if (retval) {
		pr_err("Failed to request RX irq: %d\n", retval);
		goto err_rx;
	}
	
	retval = request_irq(irq_base + PMB8876_UART_LS_IRQ_OFF, pmb8876uart_linest_handler, 0, uart_ls, port);
	if (retval)
		goto err_ls;

	/*retval = request_irq(PMB8876_IRQ_UART_MODEM_STATUS, pmb8876uart_modem_status, 0, "UART ModemStatus", port);
	if (retval)
		goto err_ms;*/
	return 0;

//err_ms:
	//free_irq(irq_base + PMB8876_UART_LS_IRQ_OFF, port);
err_ls:
	free_irq(irq_base + PMB8876_UART_RX_IRQ_OFF, port);
err_rx:
	free_irq(irq_base + PMB8876_UART_TX_IRQ_OFF, port);
err_tx:
	return retval;
}

static void pmb8876uart_shutdown(struct uart_port *port)
{
	struct pmb8876_usrptr *usrptr = port->private_data;
	u32 irq_base = port->irq;
	
	/* mask rx/tx interrupt */
	__raw_writel( 0, (void *)PMB8876_USART_IMSC(port->mapbase) );
	
	pr_info("-> Shutdown %d\n", irq_base);
	
	free_irq(irq_base + PMB8876_UART_RX_IRQ_OFF, port);
	free_irq(irq_base + PMB8876_UART_TX_IRQ_OFF, port);
	free_irq(irq_base + PMB8876_UART_LS_IRQ_OFF, port);
	//free_irq(PMB8876_IRQ_UART_MODEM_STATUS, port);
	
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

	//baud = 115200;
	
	if( usrptr && usrptr->baudrate == baud )
		new = 0;
	
	if( new ) {
		int bgfd = pmb8876_baud_to_divisor(baud);
		if( bgfd < 0 ) {
			//pr_err("Unsupported baudrate %d\n", baud);
			return;
		}
			
		pr_info("ttyAM%d: Setting baudrate %d\n", 
			port->irq == PMB8876_UART0_TX_IRQ? 0 : 1, baud);
		
		if( port->irq == PMB8876_UART1_TX_IRQ ) {
		    __raw_writel( ((bgfd >> 16)), (void *)PMB8876_USART_BG(port->mapbase) );
		    __raw_writel( ((bgfd << 16) >> 16), (void *)PMB8876_USART_FDV(port->mapbase) );
		}
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

static struct uart_port pmb8876uart_ports[] = {
	{
		.membase	= (void *)PMB8876_USART0_BASE,
		.mapbase	= PMB8876_USART0_BASE,
		.iotype		= SERIAL_IO_MEM,
		.irq		= PMB8876_UART0_TX_IRQ,
		.uartclk	= PMB8876_CLOCK_RATE * 16,
		.fifosize	= PMB8876_UART_FIFO_SIZE,
		.ops		= &pmb8876uart_pops,
		.flags		= UPF_BOOT_AUTOCONF,
		.line		= 0,
		.type		= PORT_PMB8876,
	},
	
	{
		.membase	= (void *)PMB8876_USART1_BASE,
		.mapbase	= PMB8876_USART1_BASE,
		.iotype		= SERIAL_IO_MEM,
		.irq		= PMB8876_UART1_TX_IRQ,
		.uartclk	= PMB8876_CLOCK_RATE * 16,
		.fifosize	= PMB8876_UART_FIFO_SIZE,
		.ops		= &pmb8876uart_pops,
		.flags		= UPF_BOOT_AUTOCONF | UPF_IOREMAP | UPF_LOW_LATENCY,
		.line		= 1,
		.type		= PORT_PMB8876,
	}
};



#ifdef CONFIG_SERIAL_PMB8876_CONSOLE
static void pmb8876_console_putchar(struct uart_port *port, int ch)
{
	unsigned long flags;
	
	local_irq_save(flags);
	
	UART_PUT_CHAR(port, ch);
	while( !(UART_FCSTAT(port) & ICR_TB) ) cpu_relax();
	__raw_writel( ICR_TB, (void *)PMB8876_USART_ICR(port->mapbase) );
	
	local_irq_restore(flags);
}

static void pmb8876_console_write(struct console *co, const char *s, u_int count)
{
	struct uart_port *port = &pmb8876uart_ports[co->index];

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
	int flow = 'x';

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	
	if (co->index < 0 || co->index >= SERIAL_PMB8876_NR)
		return -EINVAL;

	port = &pmb8876uart_ports[co->index];
	if( !port->membase )
		return -ENODEV;
	
	port = uart_get_console(pmb8876uart_ports, co->index, co);

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
	.flags		= CON_PRINTBUFFER,
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

static int __init
pmb8876_serial_early_console_setup(struct earlycon_device *device, const char *opt)
{
	if (!device->port.membase)
		return -ENODEV;

	device->con->write = pmb8876_console_write;
	return 0;
}
OF_EARLYCON_DECLARE(PMB8876, "PMB8876,pmb8876-uart",
		    pmb8876_serial_early_console_setup);

#define PMB8876_CONSOLE	&pmb8876_console
#else
#define PMB8876_CONSOLE	NULL
#endif

static struct uart_driver pmb8876_reg = {
	.owner			= THIS_MODULE,
	.driver_name		= "serial_pmb8876",
	.dev_name		= SERIAL_PMB8876_DEVNAME,
	.major			= SERIAL_PMB8876_MAJOR,
	.minor			= SERIAL_PMB8876_MINOR,
	.nr			= SERIAL_PMB8876_NR,
	.cons			= PMB8876_CONSOLE,
};

static int __init pmb8876uart_init(void)
{
	int i, ret;

	printk("Serial: PMB8876 UART driver\n");

	ret = uart_register_driver(&pmb8876_reg);
	if (ret)
		return ret;

	for (i = 0; i < SERIAL_PMB8876_NR; i++) {
		uart_add_one_port(&pmb8876_reg, &pmb8876uart_ports[i]);
	}
	
	return 0;
}

static void __exit pmb8876uart_exit(void)
{
	int i;

	for (i = 0; i < SERIAL_PMB8876_NR; i++)
		uart_remove_one_port(&pmb8876_reg, &pmb8876uart_ports[i]);
	uart_unregister_driver(&pmb8876_reg);
}

module_init(pmb8876uart_init);
module_exit(pmb8876uart_exit);

MODULE_DESCRIPTION("PMB8876 serial port driver");
MODULE_AUTHOR("Siemens Narkomans (c).");
MODULE_LICENSE("GPL");
