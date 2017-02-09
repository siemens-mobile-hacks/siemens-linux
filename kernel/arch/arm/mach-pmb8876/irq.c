#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/pmb8876_platform.h>
#include <asm/mach-types.h>



static void
pmb8876_irq_disable(struct irq_data *d)
{
	writel(PMB8876_GSM_TPU_MASK, PMB8876_IRQ_ADDR(d->irq) );
}


static void
pmb8876_irq_ack(struct irq_data *d)
{
	// ack
	writel(1, PMB8876_IRQ_ACK_ADDR);
}

static void
pmb8876_irq_mask(struct irq_data *d)
{
	writel(PMB8876_GSM_TPU_MASK, PMB8876_IRQ_ADDR(d->irq) );
}


static void
pmb8876_irq_unmask(struct irq_data *d)
{
	writel(PMB8876_GSM_TPU_UNMASK, PMB8876_IRQ_ADDR(d->irq) );
}


struct irq_chip ext_chip = {
	.name		= "PMB8876",
	.irq_disable=pmb8876_irq_disable,
	.irq_ack	= pmb8876_irq_ack,
	.irq_mask	= pmb8876_irq_mask,
	.irq_unmask	= pmb8876_irq_unmask,
};


void __init pmb8876_init_irq(void)
{
	int i;
	pr_info("Initialize pmb8876 irq's...\n");
	
	for (i = 0; i < NR_IRQS; i++) {
		irq_set_chip_and_handler(i, &ext_chip, handle_level_irq);
	}
}
