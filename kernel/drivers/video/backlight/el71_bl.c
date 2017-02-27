/*
 *  LCD / Backlight control code for Siemens EL71
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <mach/pmb8876-i2c.h>
#include <mach/pmb8876-gpio.h>


struct el71_bl_data {
	struct i2c_client *i2c;
	struct backlight_device *bl;

	int powered;
};

static void el71_bl_set_backlight(struct el71_bl_data *data, int brightness)
{
	if( brightness && !data->powered ) {
	    i2c_smbus_write_byte_data(data->i2c, D1601AA_LED_CONTROL, 
				      D1601AA_LED2_EN | D1601AA_LIGHT_PWM1_EN);
	    data->powered = 1;
	    //pr_info("PowerON backlight\n");
	    
	} else if(!brightness && data->powered) {
	    i2c_smbus_write_byte_data(data->i2c, D1601AA_LIGHT_PWM1, 0);
	    i2c_smbus_write_byte_data(data->i2c, D1601AA_LED_CONTROL, 0);
	    data->powered = 0;
	    //pr_info("PowerOFF backlight\n");
	    return;
	}
	
	if( brightness < 0 || brightness > 0x64 )
	    return;
	
	//pr_info("Set backlight -> %d\n", brightness);
	i2c_smbus_write_byte_data(data->i2c, D1601AA_LIGHT_PWM1, brightness);
}

static int el71_bl_update_status(struct backlight_device *dev)
{
	struct backlight_properties *props = &dev->props;
	struct el71_bl_data *data = bl_get_data(dev);
	int power = max(props->power, props->fb_blank);
	int brightness = props->brightness;

	if (power)
		brightness = 0;

	el71_bl_set_backlight(data, brightness);

	return 0;
}

static int el71_bl_get_brightness(struct backlight_device *dev)
{
	struct backlight_properties *props = &dev->props;

	return props->brightness;
}

static const struct backlight_ops bl_ops = {
	.get_brightness		= el71_bl_get_brightness,
	.update_status		= el71_bl_update_status,
};

static int el71_bl_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct backlight_properties props;
	struct el71_bl_data *data;
	int ret = 0;

	//pr_info("Probe backlight device...\n");
	data = devm_kzalloc(&client->dev, sizeof(struct el71_bl_data),
				GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->powered = 0;

	i2c_set_clientdata(client, data);
	data->i2c = client;
	
	// disable flash light
	ret = gpio_request(GPIO_LED_FL_EN, "GPIO_LED_FL_EN");
	gpio_direction_output(GPIO_LED_FL_EN, 0);
	gpio_free(ret);
	
	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = 0x64;
	data->bl = devm_backlight_device_register(&client->dev, "el71-bl",
						&client->dev, data, &bl_ops,
						&props);
	if (IS_ERR(data->bl)) {
		ret = PTR_ERR(data->bl);
		goto err_reg;
	}

	data->bl->props.brightness = 0;
	data->bl->props.power = FB_BLANK_UNBLANK;

	backlight_update_status(data->bl);

	return 0;

err_reg:
	data->bl = NULL;
	return ret;
}

static int el71_bl_remove(struct i2c_client *client)
{
	struct el71_bl_data *data = i2c_get_clientdata(client);

	data->bl = NULL;
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int el71_bl_suspend(struct device *dev)
{
	struct el71_bl_data *data = dev_get_drvdata(dev);

	el71_bl_set_backlight(data, 0);

	return 0;
}

static int el71_bl_resume(struct device *dev)
{
	struct el71_bl_data *data = dev_get_drvdata(dev);

	backlight_update_status(data->bl);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(el71_bl_pm_ops, el71_bl_suspend, el71_bl_resume);

static const struct i2c_device_id el71_bl_id[] = {
	{ "el71-bl", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, el71_bl_id);

static struct i2c_driver el71_bl_driver = {
	.driver = {
		.name		= "el71-bl",
		.owner		= THIS_MODULE,
		.pm		= &el71_bl_pm_ops,
	},
	.probe		= el71_bl_probe,
	.remove		= el71_bl_remove,
	.id_table	= el71_bl_id,
};

module_i2c_driver(el71_bl_driver);

MODULE_AUTHOR("zvova7890 zvova7890@gmail.com");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("LCD/Backlight control for Siemens EL71");

