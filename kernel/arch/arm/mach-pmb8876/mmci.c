#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/bitops.h>
#include <mach/pmb8876-gpio.h>
#include <linux/amba/bus.h>
#include <linux/amba/mmci.h>
#include <linux/mmc/host.h>

static int mmc_handle_ios(struct device *dev, struct mmc_ios *ios) {
	gpio_set_value(GPIO_MMC_VCC_EN, ios->power_mode == MMC_POWER_OFF ? 0 : 1);
	return 0;
}
static struct mmci_platform_data mmci_data = {
	.ocr_mask		= MMC_VDD_165_195 | MMC_VDD_20_21 | MMC_VDD_21_22 | MMC_VDD_22_23 | MMC_VDD_23_24 | MMC_VDD_24_25 | MMC_VDD_25_26 | MMC_VDD_26_27 | MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_34_35 | MMC_VDD_35_36, 
	.ios_handler    = mmc_handle_ios, 
	.gpio_cd		= GPIO_MMC_CD, 
	.cd_invert		= 1
};

// 0x00041180 - ид разновидности железки в mmci.c
static AMBA_APB_DEVICE(mmc0, "pmb8876:mmc0", 0x00041180, 0xF7301000, {PMB8876_MMCI_IRQ}, &mmci_data);
static struct amba_device *amba_devs[] __initdata = {
	&mmc0_device,
};

static int __init pmb8876_mmci_init(void) {
	int i;
	
	gpio_request(GPIO_MMC_CLK, "GPIO_MMC_CLK");
	gpio_request(GPIO_MMC_DAT, "GPIO_MMC_DAT");
	gpio_request(GPIO_MMC_CMD, "GPIO_MMC_CMD");
	gpio_request(GPIO_MMC_VCC_EN, "GPIO_MMC_VCC_EN");
	
	pr_info("pmb8876_mmci_init start\n");
	
	// writel(readl((void *) 0xF7300000) & ~3 | 2, (void *) 0xF7300000); // что-то из паршивки
	
	// максимальный размер карточки, наверно
	writel(1024, (void *)0xF7300000);
	
	for (i = 0; i < ARRAY_SIZE(amba_devs); ++i) {
		struct amba_device *d = amba_devs[i];
		amba_device_register(d, &iomem_resource);
	}
	pr_info("pmb8876_mmci_init end\n");
	return 0;
}
arch_initcall(pmb8876_mmci_init);
