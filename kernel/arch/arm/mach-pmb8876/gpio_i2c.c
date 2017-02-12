#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <mach/pmb8876-gpio.h>

static struct i2c_gpio_platform_data plat_gpio_i2c = {
	.sda_pin = GPIO_I2C_SDA,
	.scl_pin = GPIO_I2C_SCL
};
static struct platform_device gpio_i2c_dev = {
	.name = "i2c-gpio",
	.id   = 0,
	.dev = {
		.platform_data  = &plat_gpio_i2c,
	}
};
static int __init pmb8876_init_i2c(void)
{
	pr_info("pmb8876_init_i2c\n");
	platform_device_register(&gpio_i2c_dev);
	
	return 0;
}
arch_initcall(pmb8876_init_i2c);
