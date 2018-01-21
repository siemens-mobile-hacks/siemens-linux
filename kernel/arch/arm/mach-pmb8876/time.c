

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>

#include <linux/time.h>
#include <linux/timex.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/hardirq.h>
#include <linux/sched.h>
#include <linux/sched_clock.h>
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
#define PMB8876_GSM_TPU_CLC			0xF6400000
#define PMB8876_GSM_TPU_CON			0xF64000F8

/* flags */
#define PMB8876_GSM_TPU_CON_RESET	0x4000
#define PMB8876_GSM_TPU_CON_ENABLE	0x1000
#define PMB8876_GSM_TPU_CLC_RMC(x)	((x << 8) & 0x127)

/* freq */
#define PMB8876_GSM_CLOCK_FREQ		21660000


/* STM */
#define PMB8876_STM_CLC				0xF4B00000
#define PMB8876_STM_ID 				0xF4B00008
#define PMB8876_STM_0				0xF4B00010

#define PMB8876_STM_CLC_RMC(x)		((x << 8) & 0x7)

#define PMB8876_STM_CLOCK_FREQ		26000000


#define MAX_DELTA					( 32767 )
#define MIN_DELTA					( 10 )


#define GSM_CON()					readl((void *)PMB8876_GSM_TPU_CON)
#define GSM_CON_SET(x)				writel(x, (void *)PMB8876_GSM_TPU_CON);



static int pmb8876_timer_type_oneshot = 0;



static int pmb8876_timer_set_next_event(unsigned long ticks,
				     struct clock_event_device *evt)
{
	u64 sleep = (ticks);
	writel(sleep, (void *)0xF6400020);
	
	GSM_CON_SET(GSM_CON() | PMB8876_GSM_TPU_CON_ENABLE);
	return 0;
}

static int pmb8876_set_periodic(struct clock_event_device *evt)
{
	pr_info("PMB8876: Setup periodic timer type\n");
	pmb8876_timer_type_oneshot = 0;
	
	/* setup periodic rate */
	writel( ((PMB8876_GSM_CLOCK_FREQ / 2) / (HZ * 10)) - 1 , (void *)0xF6400020);
	
	/* reset and enable the timer */
	GSM_CON_SET(GSM_CON() | PMB8876_GSM_TPU_CON_RESET | PMB8876_GSM_TPU_CON_ENABLE);
	
	/* enable irq */
	writel(pmb8876_irq_priority(PMB8876_GSM_TIMER_IRQ), (void *)(PMB8876_IRQ_ADDR(PMB8876_GSM_TIMER_IRQ)));
	return 0;
}

static int pmb8876_timer_oneshot(struct clock_event_device *evt)
{	
	pr_info("PMB8876: Setup oneshot timer type(dynticks)\n");
	pmb8876_timer_type_oneshot = 1;
	
	/* disable timer */
	GSM_CON_SET(GSM_CON() & ~PMB8876_GSM_TPU_CON_ENABLE);
	
	/* enable irq */
	writel(pmb8876_irq_priority(PMB8876_GSM_TIMER_IRQ), (void *)(PMB8876_IRQ_ADDR(PMB8876_GSM_TIMER_IRQ)));
	return 0;
}

/*
 * Whenever anyone tries to change modes, we just mask interrupts
 * and wait for the next event to get set.
 */
static int pmb8876_timer_shutdown(struct clock_event_device *evt)
{
	/* disable irq */
	writel(PMB8876_IRQ_MASK, (void *)(PMB8876_IRQ_ADDR(PMB8876_GSM_TIMER_IRQ)));
	
	/* disable timer */
	GSM_CON_SET(GSM_CON() & ~PMB8876_GSM_TPU_CON_ENABLE);
	return 0;
}

static struct clock_event_device clockevent_pmb8876 = {
	.name			= "PMB8876 clockevent",
	.features		= CLOCK_EVT_FEAT_ONESHOT | CLOCK_EVT_FEAT_PERIODIC,
	.rating			= 100,
	.irq			= PMB8876_GSM_TIMER_IRQ,
	
	.set_next_event		= pmb8876_timer_set_next_event,
	.set_state_periodic	= pmb8876_set_periodic,
	.set_state_shutdown	= pmb8876_timer_shutdown,
	.set_state_oneshot	= pmb8876_timer_oneshot,
};




static u64 read_sched_clock(void)
{
    u64 cyc;
    //unsigned long flags;
    
    //local_irq_save(flags);
    cyc = __raw_readl((void *)PMB8876_STM_0);
    //local_irq_restore(flags);
    return cyc;
}

static u64 clksrc_read(struct clocksource *cs)
{
    return read_sched_clock();
}

static struct clocksource cksrc = {
	.name		= "PMB8876 clocksource",
	.rating		= 200,
	.read		= clksrc_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};



/*
 * Setup timer clock event device
 */
void __init setup_pmb8876_timer(void)
{
	struct clock_event_device *evt = (&clockevent_pmb8876);
	unsigned int i = 0;
	u32 stm_clc;
	
	/* Mark as being for this cpu only. */
	evt->cpumask = cpumask_of(smp_processor_id());
	
	
	/* 
	 * init STM clock source 
	 * bit16 - хз
	 */
	//writel( PMB8876_STM_CLC_RMC(1), (void *)PMB8876_STM_CLC);
	stm_clc = readl((void *)PMB8876_STM_CLC);
	pr_info("STM CLC: flags %X, div %d\n", stm_clc, (stm_clc >> 8) & 0x7);
	
	/* 
	 * init GSM timer
	 */
	writel(PMB8876_GSM_TPU_CLC_RMC(1), (void *)PMB8876_GSM_TPU_CLC);
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

	writel( ((PMB8876_GSM_CLOCK_FREQ / 2) / (HZ * 10)) - 1 , (void *)0xF6400020);
	
	writel(0, (void *)0xF640002C);
	writel(0, (void *)0xF6400024);
	
	// WTF addr in IntRAM
	writel(0x7530, (void *)0xF6400028);
	writel(3, (void *)0xF640005C);
	
	/* Start out with timer not firing. */
	writel(PMB8876_IRQ_MASK, PMB8876_IRQ_ADDR(PMB8876_GSM_TIMER_IRQ) );
	
	/* add external clock source */
	clocksource_register_hz(&cksrc, PMB8876_STM_CLOCK_FREQ);
	
	/* set scheduler clocking */
	sched_clock_register(read_sched_clock, 32, PMB8876_STM_CLOCK_FREQ);
	
	/* Register pmb8876 timer. */
	clockevents_config_and_register(evt, PMB8876_GSM_CLOCK_FREQ,
					MIN_DELTA, MAX_DELTA);
}


static irqreturn_t pmb8876_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = (&clockevent_pmb8876);
	GSM_CON_SET(GSM_CON() | PMB8876_GSM_TPU_CON_RESET);
	
	if( pmb8876_timer_type_oneshot ) {
	    GSM_CON_SET(GSM_CON() & ~PMB8876_GSM_TPU_CON_ENABLE);
	}
	
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

