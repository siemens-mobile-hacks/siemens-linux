/*
 * Copyright (C) 2010 Google, Inc.
 *
 * Author:
 *	Colin Cross <ccross@google.com>
 *	Based on arch/arm/plat-omap/cpu-omap.c, (C) 2005 Nokia Corporation
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>


int pmb8876_get_cpu_rate(void);
int pmb8876_set_cpu_rate(int mhz);

static struct cpufreq_frequency_table freq_table[] = {
	{ .frequency = 12000 },
	{ .frequency = 18000 },
	{ .frequency = 26000 },
	{ .frequency = 52000 },
	{ .frequency = 78000 },
	{ .frequency = 104000 },
	{ .frequency = 156000 },
	{ .frequency = 208000 },
	{ .frequency = 260000 },
	{ .frequency = 312000 },
	{ .frequency = CPUFREQ_TABLE_END },
};

#define NUM_CPUS	1



static unsigned int pmb8876_get_cpu_frequency(unsigned int cpu)
{
	return pmb8876_get_cpu_rate();
}


static int pmb8876_target_index(struct cpufreq_policy *policy,
			  unsigned int index)
{
	unsigned long rate = freq_table[index].frequency;

	if ((rate > policy->max) || (rate < policy->min))
		return -EINVAL;
	
	return pmb8876_set_cpu_rate(rate);
}
/*
static int pmb8876_target(struct cpufreq_policy *policy,
			  unsigned int target_freq, unsigned int relation)
{
	//unsigned long rate = freq_table[index].frequency;
	unsigned long rate  = target_freq;
	struct cpufreq_freqs freqs;
	
	if ((target_freq > policy->max) || (target_freq < policy->min))
		return -EINVAL;
	
	freqs.old = pmb8876_pll_get_cpu_rate();
	freqs.new = rate;
	freqs.flags = 0;
	
	
	cpufreq_freq_transition_begin(policy, &freqs);
	
	switch(rate)
	{
	    case 26000:
		pmb8876_pll_reclock(1);
		break;
	    
	    case 78000:
		pmb8876_pll_reclock(2);
		break;
		
	    case 104000:
		pmb8876_pll_reclock(3);
		break;
		
	    case 130000:
		pmb8876_pll_reclock(4);
		break;
		
	    case 156000:
		pmb8876_pll_reclock(5);
		break;
		
	    default:
		cpufreq_freq_transition_end(policy, &freqs, 1);
		return -1;
	}
	
	cpufreq_freq_transition_end(policy, &freqs, 0);
	pr_info("Set cpu rate: %d\n", rate);
	return 0;
}
*/
static int pmb8876_cpu_init(struct cpufreq_policy *policy)
{
	int ret;

	if (policy->cpu >= NUM_CPUS)
		return -EINVAL;

	pr_info("Init CPU freq driver...\n");

	policy->min = 12000;
	policy->max = 312000;
	/* FIXME: what's the actual transition time? */
	ret = cpufreq_generic_init(policy, freq_table, 300 * 1000);
	if (ret) {
		return ret;
	}

	return 0;
}



static struct cpufreq_driver pmb8876_cpufreq_driver = {
	.flags			= CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify			= cpufreq_generic_frequency_table_verify,
	//.target			= pmb8876_target,
	.target_index		= pmb8876_target_index,
	.get			= pmb8876_get_cpu_frequency,
	.init			= pmb8876_cpu_init,
	.name			= "PMB8876",
};

static int __init pmb8876_cpufreq_init(void)
{
	return cpufreq_register_driver(&pmb8876_cpufreq_driver);
}

static void __exit pmb8876_cpufreq_exit(void)
{
        cpufreq_unregister_driver(&pmb8876_cpufreq_driver);
}


MODULE_AUTHOR("zvova7890 <zvova7890@gmail.com>");
MODULE_DESCRIPTION("cpufreq driver for PMB8876 SoC");
MODULE_LICENSE("GPL");
module_init(pmb8876_cpufreq_init);
module_exit(pmb8876_cpufreq_exit);
