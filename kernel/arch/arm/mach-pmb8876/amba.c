#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/amba/bus.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>

static int __init pmb8876_amba_init(void) {
	struct clk *clk;
	
	pr_info("%s()\n", __func__);
	
	// Dummy amba clk
	clk = clk_register_fixed_rate(NULL, "apb_pclk", NULL, 0, 0);
	clk_register_clkdev(clk, "apb_pclk", NULL);
	
	return 0;
}
arch_initcall(pmb8876_amba_init);
