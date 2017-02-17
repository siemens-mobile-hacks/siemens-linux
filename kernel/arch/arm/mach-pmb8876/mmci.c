#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/bitops.h>
#include <mach/pmb8876-gpio.h>
#include <linux/amba/bus.h>
#include <linux/amba/mmci.h>
#include <linux/mmc/host.h>

static int mmc_handle_ios(struct device *dev, struct mmc_ios *ios) {
	pr_info("mmc_handle_ios=%d\n", ios->power_mode == MMC_POWER_OFF ? 0 : 1);
	gpio_set_value(GPIO_MMC_VCC_EN, ios->power_mode == MMC_POWER_OFF ? 0 : 1);
	return 0;
}
static struct mmci_platform_data mmci_data = {
	.ios_handler    = mmc_handle_ios, 
	.gpio_cd		= GPIO_MMC_CD
};

// 0x00041180 - ид разновидности железки в mmci.c
static AMBA_APB_DEVICE(mmc0, "pmb8876:mmc0", 0x00041180, 0xF7301000, {0}, &mmci_data);
static struct amba_device *amba_devs[] __initdata = {
	&mmc0_device,
};

static int __init pmb8876_mmci_init(bool is_cp) {
	gpio_request(GPIO_MMC_CLK, "GPIO_MMC_CLK");
	gpio_request(GPIO_MMC_DAT, "GPIO_MMC_DAT");
	gpio_request(GPIO_MMC_CMD, "GPIO_MMC_CMD");
	gpio_request(GPIO_MMC_VCC_EN, "GPIO_MMC_VCC_EN");
	
	pr_info("pmb8876_mmci_init start\n");
	writel(readl((void *) 0xF7300000) & ~3 | 2, (void *) 0xF7300000); // что-то из паршивки
	int i;
	for (i = 0; i < ARRAY_SIZE(amba_devs); ++i) {
		struct amba_device *d = amba_devs[i];
		amba_device_register(d, &iomem_resource);
	}
	pr_info("pmb8876_mmci_init end\n");
}
arch_initcall(pmb8876_mmci_init);
