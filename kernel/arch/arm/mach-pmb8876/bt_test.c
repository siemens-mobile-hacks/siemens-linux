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

#include <linux/slab.h>
#include <linux/delay.h>
#include <mach/pmb8876-i2c.h>


// Тестируем i2c
static int __init pmb8876_test_i2c_bt(void)
{
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	int ret;
	
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
	
	if( (ret = i2c_smbus_write_byte_data(client, D1601AA_RF_REG, D1601AA_VRF3 | D1601AA_VRF2)) ) {
	    pr_err(" -> Failed to powerup BT!!! %d\n", ret);
	    
	} else {
	    pr_info("BT seems is powered ON\n");
	}
	
	kzfree(client);
	i2c_put_adapter(adapter);
	
	return 0;
}
subsys_initcall_sync(pmb8876_test_i2c_bt);
