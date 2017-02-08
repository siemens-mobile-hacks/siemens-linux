

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
#include <mach/pmb8876_platform.h>
#include <asm/mach-types.h>


static unsigned int lastticks = 0;


#define PMB8876_GSM_CLOCK_FREQ 9999


static int pmb8876_timer_set_next_event(unsigned long ticks,
				     struct clock_event_device *evt)
{
	writel(PMB8876_GSM_TPU_UNMASK, PMB8876_IRQ_ADDR(PMB8876_GSM_TIMER) );
	return 0;
}

/*
 * Whenever anyone tries to change modes, we just mask interrupts
 * and wait for the next event to get set.
 */
static int pmb8876_timer_shutdown(struct clock_event_device *evt)
{
	writel(PMB8876_GSM_TPU_MASK, PMB8876_IRQ_ADDR(PMB8876_GSM_TIMER) );
	return 0;
}

/*
 * Set min_delta_ns to 0.10 seconds, since it takes about
 * that long to fire the interrupt.
 */
static struct clock_event_device clockevent_pmb8876 = {
	.name = "PMB8876 Timer",
	.features = CLOCK_EVT_FEAT_ONESHOT,
	/*.min_delta_ns = 10000,
	.rating = 100,
	.irq = -1,*/
	.set_next_event = pmb8876_timer_set_next_event,
	.set_state_shutdown = pmb8876_timer_shutdown,
	.set_state_oneshot = pmb8876_timer_shutdown,
	.tick_resume = pmb8876_timer_shutdown,
};


/*
 * Setup timer clock event device
 * 
 * PS. Хуй знает, что там за магические значения ему надо, всё от фонаря, от них вроде кардинально ничего не меняется.
 *     Меняется только замена частоты таймера(переменная HZ) в настройках.
 */
void setup_pmb8876_timer(void)
{
	struct clock_event_device *evt = (&clockevent_pmb8876);

	/* Fill in fields that are speed-specific. */
	//clockevents_calc_mult_shift(evt, 9999, 1);
	//evt->max_delta_ns = 1000;

	/* Mark as being for this cpu only. */
	evt->cpumask = cpumask_of(smp_processor_id());

	/* Start out with timer not firing. */
	writel(PMB8876_GSM_TPU_MASK, PMB8876_IRQ_ADDR(PMB8876_GSM_TIMER) );

	/* Register pmb8876 timer. */
	clockevents_config_and_register(evt, PMB8876_GSM_CLOCK_FREQ,
					100000000, 0xfffffffe);
}


/*
 * Timer IRQ handler. Steps:
 * 	1. Reset timer
 * 	2. Run event handler
 */
static irqreturn_t pmb8876_timer_interrupt(int irq, void *dev_id)
{
	extern unsigned long last_watchdog_time;
	struct clock_event_device *evt = (&clockevent_pmb8876);
	unsigned int ticks;
	
	// reset
	writel(9999, (void *)0xF6400020);
	writel(0x00005000, (void *)0xF64000F8);
	
	// read tick value
	ticks = readl((void *)0xf4b00020);
	
	evt->event_handler(evt);

	lastticks = ticks;
	
	return IRQ_HANDLED;
}


static struct irqaction pmb8876_timer_irq = {
	.name		= "PMB8876 GSM Timer",
	.flags		= IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= pmb8876_timer_interrupt,
};




static struct timer_list pmb8876_watchdog_timer;
void pmb8876_timer_callback( unsigned long data )
{
	int st = 1;

	{
		unsigned int r2 = readl((void *)SIEMENS_EL71_EXT_WATCHDOG);
		unsigned int r0 = r2 << 22;
		r0 = r0 >> 31;
		r0 = ~r0;
		r0 = r0 & 1;
		r0 = r0 << 9;

		r2 = r2 & ~0x200;

		writel(r0 | r2, (void *)SIEMENS_EL71_EXT_WATCHDOG);
	}
	//pr_info("Serve Watchdog...\n");
	
	mod_timer(&pmb8876_watchdog_timer, jiffies + msecs_to_jiffies(1400));
}



void __init pmb8876_init_time(void)
{
	pr_info("Initialize pmb8876 timer...\n");
	
	setup_pmb8876_timer();
	
	lastticks = readl((void *)0xf4b00020);
	setup_irq(0x77, &pmb8876_timer_irq);
	
	
	/* 
	 *  Watchdog keep-alive timer
	 */
	setup_timer(&pmb8876_watchdog_timer, pmb8876_timer_callback, 0);
	mod_timer(&pmb8876_watchdog_timer, jiffies + msecs_to_jiffies(50));
}

