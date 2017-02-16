#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H



#define PMB8876_IRQ_ACK_ADDR		((void *)0xF2800014)	
#define PMB8876_IRQ_BASE		0xf2800030
#define PMB8876_IRQ_ADDR(n) 		((void *)PMB8876_IRQ_BASE + ((n) * 4))
#define PMB8876_IRQ_MASK		0x0

#define PMB8876_UART_TX_IRQ		4
#define PMB8876_UART_RX_IRQ		6
#define PMB8876_UART_LS_IRQ		7		/* line status interrupt */
#define PMB8876_GSM_TIMER_IRQ		119		/* clockevent timer */

#define NR_IRQS			0x9d

int  pmb8876_irq_priority(int irq);
void pmb8876_set_irq_priority(int irq, unsigned char priority);


#endif
