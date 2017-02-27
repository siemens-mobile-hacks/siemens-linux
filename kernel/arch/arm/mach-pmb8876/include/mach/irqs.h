#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H



#define PMB8876_IRQ_ACK_ADDR		((void *)0xF2800014)	
#define PMB8876_IRQ_BASE		0xf2800030
#define PMB8876_IRQ_ADDR(n) 		((void *)PMB8876_IRQ_BASE + ((n) * 4))
#define PMB8876_IRQ_MASK		0x0


#define PMB8876_MMCI_IRQ		0x94

#define PMB8876_UART0_TX_IRQ		4
#define PMB8876_UART0_RX_IRQ		6
#define PMB8876_UART0_LS_IRQ		7		/* line status interrupt */

#define PMB8876_UART1_TX_IRQ		0x1A
#define PMB8876_UART1_RX_IRQ		0x1C
#define PMB8876_UART1_LS_IRQ		0x1D		/* line status interrupt */

#define PMB8876_UART_TX_IRQ_OFF		0
#define PMB8876_UART_RX_IRQ_OFF		2
#define PMB8876_UART_LS_IRQ_OFF		3		/* line status interrupt */


#define PMB8876_GSM_TIMER_IRQ		119		/* clockevent timer */

#define NR_IRQS			0x9d

int  pmb8876_irq_priority(int irq);
void pmb8876_set_irq_priority(int irq, unsigned char priority);


#endif
