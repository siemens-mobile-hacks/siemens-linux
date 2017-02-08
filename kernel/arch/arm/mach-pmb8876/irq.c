#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/pmb8876_platform.h>
#include <asm/mach-types.h>



static void
pmb8876_timer_irq_enable(struct irq_data *d)
{
	// TODO ???
	pr_info(" -> %s(%d)!\n",  __func__, d->irq);
	
	if( d->irq == 0x77 ) {
		writel(0x100, (void *)0xF6400000);
		writel(9999, (void *)0xF6400020);
		writel(0x00000000, (void *)0xF6400024);
		writel(0x00007530, (void *)0xF6400028);
		writel(0x00000000, (void *)0xF640002C);
		writel(0x00000006, (void *)0xF640003C);
		writel(0x00000000, (void *)0xF6400040);
		writel(0x80000000, (void *)0xF6400044);
		writel(0x00000003, (void *)0xF640005C);
		writel(0x00000001, (void *)0xF6400068);
		writel(0x00000004, (void *)0xF640006C);
		writel(0x00000002, (void *)0xF6400070);
		writel(0x00004000, (void *)0xF64000F8);
		writel(0x00005000, (void *)0xF64000F8);
	}
	
	writel(PMB8876_GSM_TPU_UNMASK, PMB8876_IRQ_ADDR(d->irq) );
}


static void
pmb8876_timer_irq_disable(struct irq_data *d)
{
	pr_info(" -> %s!\n",  __func__);
	writel(PMB8876_GSM_TPU_MASK, PMB8876_IRQ_ADDR(0x77) );
	writel(0x0, (void *)0xF6400000);
}


static void
pmb8876_timer_irq_ack(struct irq_data *d)
{
	// ack
	writel(1, PMB8876_IRQ_ACK_ADDR);
	
	//pr_info(" -> %s!\n",  __func__);
}


static void
pmb8876_timer_irq_mask(struct irq_data *d)
{
	writel(PMB8876_GSM_TPU_MASK, PMB8876_IRQ_ADDR(0x77) );
	//pr_info(" -> %s!\n",  __func__);
}


static void
pmb8876_timer_irq_unmask(struct irq_data *d)
{
	writel(PMB8876_GSM_TPU_UNMASK, PMB8876_IRQ_ADDR(0x77) );
	//pr_info(" -> %s!\n",  __func__);
}


struct irq_chip ext_chip = {
	.name		= "PMB8876",
	//.irq_startup= pmb8876_timer_irq_startup,
	.irq_enable	= pmb8876_timer_irq_enable,
	.irq_disable= pmb8876_timer_irq_disable,
	.irq_ack	= pmb8876_timer_irq_ack,
	.irq_mask	= pmb8876_timer_irq_mask,
	.irq_unmask	= pmb8876_timer_irq_unmask,
};


void __init pmb8876_init_irq(void)
{
	int i;
	pr_info("Initialize pmb8876 irq's...\n");
	
	for (i = 0; i < NR_IRQS; i++) {
		irq_set_chip_and_handler(i, &ext_chip, handle_level_irq);
		//pmb8876_serve_watchdog();
	}
}
