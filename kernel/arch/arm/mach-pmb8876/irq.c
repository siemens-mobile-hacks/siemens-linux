#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/pmb8876_platform.h>
#include <asm/mach-types.h>


struct irq_chip_data
{
	unsigned char priority;
};

struct irq_chip_data irqs_data[NR_IRQS];


#define IRQ_PRIORITY(n) ((n > -1 && n < NR_IRQS)? irqs_data[n].priority : 1 )





int 
pmb8876_irq_priority(int irq)
{
	return IRQ_PRIORITY(irq);
}


void
pmb8876_set_irq_priority(int irq, unsigned char priority)
{
	if( irq < 0 || irq >= NR_IRQS ) {
		pr_warn("%s: Invalid irq number specified %d\n", __func__, irq);
		return;
	}
	
	irqs_data[irq].priority = priority;
}



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
	//if(d->irq != 0x77)
	//	pr_info(" -> Unmask %d irq!\n", d->irq);
	writel(IRQ_PRIORITY(d->irq), PMB8876_IRQ_ADDR(d->irq) );
}


struct irq_chip ext_chip = {
	.name		= "PMB8876",
	.irq_disable= pmb8876_irq_disable,
	.irq_ack	= pmb8876_irq_ack,
	.irq_mask	= pmb8876_irq_mask,
	.irq_unmask	= pmb8876_irq_unmask,
};


void __init pmb8876_init_irq(void)
{
	int i, clr = 0;
	pr_info("Initialize pmb8876 irq's...\n");
	
	for (i = 0; i < NR_IRQS; i++) {
		clr = IRQ_NOREQUEST;
		
		/* mask irq */
		writel(PMB8876_GSM_TPU_MASK, PMB8876_IRQ_ADDR(i));
		
		irq_set_chip_and_handler(i, &ext_chip, handle_level_irq);
		irq_modify_status(i, clr, 0);
		
		irqs_data[i].priority = 1;
	}
}
