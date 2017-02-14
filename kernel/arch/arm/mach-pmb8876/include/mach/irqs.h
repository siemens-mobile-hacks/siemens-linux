#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

#define NR_IRQS					0x78




int  pmb8876_irq_priority(int irq);
void pmb8876_set_irq_priority(int irq, unsigned char priority);


#endif
