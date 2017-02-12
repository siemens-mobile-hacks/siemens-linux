#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <mach/pmb8876-i2c.h>
#include <mach/pmb8876-gpio.h>

static void test_vibra(struct i2c_client *client) {
	int ret, i;
	
	pr_info("Testing i2c vibra...\n");
	
	for (i = 0; i < 0x64; ++i) {
		if ((ret = i2c_smbus_write_byte_data(client, D1601AA_VIBRA, i)) != 0) {
			pr_info("%s: fail: %d\n", __func__, ret);
			return;
		}
		mdelay(30);
	}
	
	for (i = 0x64; i-- > 0; ) {
		if ((ret = i2c_smbus_write_byte_data(client, D1601AA_VIBRA, i)) != 0) {
			pr_info("%s: fail: %d\n", __func__, ret);
			return;
		}
		mdelay(30);
	}
}

static void test_pickoff_sound(struct i2c_client *client) {
	int ret;
	
	pr_info("Testing pickoff sound...\n");
	
	if ((ret = i2c_smbus_write_byte_data(client, 0x44, 0x24)) != 0) {
		pr_info("%s: fail: %d\n", __func__, ret);
		return;
	}
	
	if ((ret = i2c_smbus_write_byte_data(client, 0x46, 0x5F)) != 0) {
		pr_info("%s: fail: %d\n", __func__, ret);
		return;
	}
	
	if ((ret = i2c_smbus_write_byte_data(client, 0x42, 0x5F)) != 0) {
		pr_info("%s: fail: %d\n", __func__, ret);
		return;
	}
}

static void test_backlight(struct i2c_client *client) {
	int ret, lcd_control = 0, i;
	
	lcd_control |= D1601AA_LED2_EN; // Даём питалово
	
	// ================= DISPLAY =================
	pr_info("Testing i2c LCD backlight...\n");
	
	lcd_control |= D1601AA_LIGHT_PWM1_EN;
	
	if ((ret = i2c_smbus_write_byte_data(client, D1601AA_LED_CONTROL, lcd_control)) != 0) {
		pr_info("%s: fail: %d\n", __func__, ret);
		return;
	}
	
	for (i = 0; i < 0x64; ++i) {
		if ((ret = i2c_smbus_write_byte_data(client, D1601AA_LIGHT_PWM1, i)) != 0) {
			pr_info("%s: fail: %d\n", __func__, ret);
			return;
		}
		mdelay(30);
	}
	
	for (i = 0x64; i-- > 0; ) {
		if ((ret = i2c_smbus_write_byte_data(client, D1601AA_LIGHT_PWM1, i)) != 0) {
			pr_info("%s: fail: %d\n", __func__, ret);
			return;
		}
		mdelay(30);
	}
	lcd_control &= ~D1601AA_LIGHT_PWM1_EN;
	
	// ================= KEYBOARD =================
	pr_info("Testing i2c keyboard backlight...\n");
	
	lcd_control |= D1601AA_LIGHT_PWM2_EN;
	
	if ((ret = i2c_smbus_write_byte_data(client, D1601AA_LED_CONTROL, lcd_control)) != 0) {
		pr_info("%s: fail: %d\n", __func__, ret);
		return;
	}
	
	for (i = 0; i < 0x64; ++i) {
		if ((ret = i2c_smbus_write_byte_data(client, D1601AA_LIGHT_PWM2, i)) != 0) {
			pr_info("%s: fail: %d\n", __func__, ret);
			return;
		}
		mdelay(30);
	}
	
	for (i = 0x64; i-- > 0; ) {
		if ((ret = i2c_smbus_write_byte_data(client, D1601AA_LIGHT_PWM2, i)) != 0) {
			pr_info("%s: fail: %d\n", __func__, ret);
			return;
		}
		mdelay(30);
	}
	lcd_control &= ~D1601AA_LIGHT_PWM2_EN;
	
	// ================= FLASH LIGHT =================
	pr_info("Testing i2c flash light...\n");
	
	
	ret = gpio_request(GPIO_LED_FL_EN, "GPIO_LED_FL_EN");
	for (i = 0; i < 8; ++i) {
		gpio_direction_output(GPIO_LED_FL_EN, 1);
		mdelay(500);
		gpio_direction_output(GPIO_LED_FL_EN, 0);
		mdelay(500);
	}
	gpio_free(ret);
}

// Тестируем i2c
static int __init pmb8876_test_i2c(void)
{
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	
	// get i2c adapter
	adapter = i2c_get_adapter(0);
	if (!adapter) {
		pr_info("i2c_get_adapter error\n");
		return -1;
	}
	
	// create i2c client
	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client) {
		pr_info("i2c_client alloc error\n");
		return -1;
	}
	client->adapter = adapter;
	client->addr = PMB8876_I2C_D1601AA;
	
	test_vibra(client);
	test_backlight(client);
	test_pickoff_sound(client);
	
	kzfree(client);
	i2c_put_adapter(adapter);
	
	pr_info("pmb8876_test_i2c - ok?\n");
	
	return 0;
}
subsys_initcall_sync(pmb8876_test_i2c);
