

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>

#include <linux/time.h>
#include <linux/timex.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/hardirq.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/timekeeper_internal.h>

#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <asm/mach-types.h>


/* register */
#define PMB8876_GSM_TPU_CON		0xF64000F8

/* flags */
#define PMB8876_GSM_TPU_CON_RESET	0x4000
#define PMB8876_GSM_TPU_CON_ENABLE	0x1000


#define PMB8876_GSM_CLOCK_FREQ		2166000


#define MAX_DELTA		(0xfffffffe)
#define MIN_DELTA		(16)


#define GSM_CON()		readl((void *)PMB8876_GSM_TPU_CON)
#define GSM_CON_SET(x)		writel(x, (void *)PMB8876_GSM_TPU_CON);




static int pmb8876_set_periodic(struct clock_event_device *evt)
{
	GSM_CON_SET(GSM_CON() | PMB8876_GSM_TPU_CON_ENABLE | PMB8876_GSM_TPU_CON_RESET);
	writel(pmb8876_irq_priority(PMB8876_GSM_TIMER_IRQ), (void *)(PMB8876_IRQ_ADDR(PMB8876_GSM_TIMER_IRQ)));
	return 0;
}

/*
static int pmb8876_timer_set_next_event(unsigned long ticks,
				     struct clock_event_device *evt)
{
	GSM_CON_SET(GSM_CON() | PMB8876_GSM_TPU_CON_ENABLE);
	return 0;
}
*/

/*
 * Whenever anyone tries to change modes, we just mask interrupts
 * and wait for the next event to get set.
 */
static int pmb8876_timer_shutdown(struct clock_event_device *evt)
{
	writel(PMB8876_IRQ_MASK, (void *)(PMB8876_IRQ_ADDR(PMB8876_GSM_TIMER_IRQ)));
	GSM_CON_SET(GSM_CON() & ~PMB8876_GSM_TPU_CON_ENABLE);
	return 0;
}


static struct clock_event_device clockevent_pmb8876 = {
	.name = "PMB8876 Timer",
	.features = /*CLOCK_EVT_FEAT_ONESHOT*/ CLOCK_EVT_FEAT_PERIODIC,
	.rating = 216,
	.irq = PMB8876_GSM_TIMER_IRQ,
	
	//.set_next_event = pmb8876_timer_set_next_event,
	.set_state_periodic	= pmb8876_set_periodic,
	.set_state_shutdown = pmb8876_timer_shutdown,
	.set_state_oneshot = pmb8876_timer_shutdown,
	.tick_resume = pmb8876_timer_shutdown,
};


/*
 * Setup timer clock event device
 */
void setup_pmb8876_timer(void)
{
	struct clock_event_device *evt = (&clockevent_pmb8876);
	unsigned int i = 0;
	
	/* Mark as being for this cpu only. */
	evt->cpumask = cpumask_of(smp_processor_id());
	
	/* init GSM timer */
	writel(256, (void *)0xF6400000);
	writel(1, (void *)0xF6400068);
	writel(4, (void *)0xF640006C);
	writel(2, (void *)0xF6400070);
	
	for( i = 0; i < 512; i ++ ) {
		writel(0, (void *)0xF6401800 + (i*4));
	}
	
	writel(65024, (void *)0xF6401800);
	writel(0, (void *)0xF6401804);
	writel(0, (void *)0xF6401808);
	writel(32256, (void *)0xF640180C);
	writel(32760, (void *)0xF6401810);
	writel(4096, (void *)0xF6401814);
	writel(0, (void *)0xF6400040);
	writel(6, (void *)0xF640003C);
	writel(0x80000000, (void *)0xF6400044);

	// magic
	writel((unsigned int)(((PMB8876_GSM_CLOCK_FREQ / (HZ * 10)) + 1) * 4.615), (void *)0xF6400020);
	
	writel(0, (void *)0xF640002C);
	writel(0, (void *)0xF6400024);
	
	// WTF addr in IntRAM
	writel(0x7530, (void *)0xF6400028);
	writel(3, (void *)0xF640005C);
	
	
	/* Start out with timer not firing. */
	writel(PMB8876_IRQ_MASK, PMB8876_IRQ_ADDR(PMB8876_GSM_TIMER_IRQ) );

	/* Register pmb8876 timer. */
	clockevents_config_and_register(evt, PMB8876_GSM_CLOCK_FREQ,
					MIN_DELTA, MAX_DELTA);
}


static irqreturn_t pmb8876_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = (&clockevent_pmb8876);
	
	GSM_CON_SET(GSM_CON() | PMB8876_GSM_TPU_CON_RESET);

	evt->event_handler(evt);
	return IRQ_HANDLED;
}


static struct irqaction pmb8876_timer_irq = {
	.name		= "PMB8876 GSM Timer",
	.flags		= IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= pmb8876_timer_interrupt,
};


void __init pmb8876_init_time(void)
{
	pr_info("Initialize pmb8876 clockevent timer...\n");
	
	setup_pmb8876_timer();
	setup_irq(PMB8876_GSM_TIMER_IRQ, &pmb8876_timer_irq);
}

