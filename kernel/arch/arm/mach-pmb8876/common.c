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
#include <linux/slab.h>
#include <linux/i2c.h>
#include <mach/pmb8876-i2c.h>
#include <mach/pmb8876-gpio.h>

/*
 * Platform devices
 */
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



/*
 * I2C devices
 */
static struct i2c_board_info __initdata siemens_i2c_devices[] = {
#if defined(CONFIG_BACKLIGHT_SEL71)
	{
		I2C_BOARD_INFO("el71-bl", PMB8876_I2C_D1601AA),
	},
#endif
};


static int __init pmb8876_init(void)
{
	//pm_power_off = pmb8876_power_off;
	i2c_register_board_info(0, siemens_i2c_devices,
				ARRAY_SIZE(siemens_i2c_devices));
	return platform_add_devices(pmb8876_devices, ARRAY_SIZE(pmb8876_devices));
}

arch_initcall(pmb8876_init); 
