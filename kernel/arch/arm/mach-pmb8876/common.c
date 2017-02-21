/*
 * arch/arm/mach-pmb8876/common.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <mach/hardware.h>


static struct resource pmb8876_rtc_resources[] = {
	DEFINE_RES_MEM(0xF4700000, 0x40),
	//DEFINE_RES_IRQ_NAMED(IRQ_RTC1Hz, "rtc 1Hz"),
	//DEFINE_RES_IRQ_NAMED(IRQ_RTCAlrm, "rtc alarm"),
};

static struct platform_device pmb8876rtc_device = {
	.name		= "pmb8876rtc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(pmb8876_rtc_resources),
	.resource	= pmb8876_rtc_resources,
};


static struct platform_device *pmb8876_devices[] __initdata = {
	&pmb8876rtc_device,
};

static int __init pmb8876_init(void)
{
	//pm_power_off = pmb8876_power_off;
	return platform_add_devices(pmb8876_devices, ARRAY_SIZE(pmb8876_devices));
}

arch_initcall(pmb8876_init); 
