/*
 * 
 *		External watchdog reset timer
 *
 */


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
#include <mach/irqs.h>
#include <asm/mach-types.h>


static struct timer_list pmb8876_watchdog_timer;
static int xuj = 0;

void pmb8876_timer_callback( unsigned long data )
{
	((void)data);
	
	//if(xuj < 50)
	{
		unsigned int r2 = readl((void *)PMB8876_EXT_WATCHDOG);
		unsigned int r0 = r2 << 22;
		r0 = r0 >> 31;
		r0 = ~r0;
		r0 = r0 & 1;
		r0 = r0 << 9;

		r2 = r2 & ~0x200;

		writel(r0 | r2, (void *)PMB8876_EXT_WATCHDOG);
		
		xuj++;
	}
	
	mod_timer(&pmb8876_watchdog_timer, jiffies + msecs_to_jiffies(1400));
}



int __init pmb8876_init_watchdog(void)
{
		
	/* 
	 *  Watchdog keep-alive timer
	 */
	setup_timer(&pmb8876_watchdog_timer, pmb8876_timer_callback, 0);
	mod_timer(&pmb8876_watchdog_timer, jiffies + msecs_to_jiffies(1000));
	return 0;
}
arch_initcall(pmb8876_init_watchdog);

