/*
 * TX4939 internal RTC driver
 * Based on RBTX49xx patch from CELF patch archive.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * (C) Copyright TOSHIBA CORPORATION 2005-2007
 */
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/gfp.h>
#include <mach/pmb8876_platform.h>


#define SCU_RTCIF		0xF4400064


struct pmb8876rtc_plat_data {
	struct rtc_device *rtc;
	void __iomem *base;
	spinlock_t lock;
};

static struct pmb8876rtc_plat_data *get_pmb8876rtc_plat_data(struct device *dev)
{
	return platform_get_drvdata(to_platform_device(dev));
}

static int pmb8876_rtc_set_time(struct device *dev, struct rtc_time *time)
{
	struct pmb8876rtc_plat_data *pdata = get_pmb8876rtc_plat_data(dev);
	unsigned long sec = 0;
	int ret = 0;
	
	rtc_tm_to_time(time, &sec);
	
	spin_lock_irq(&pdata->lock);
	writel(sec, (void *)pdata->base + 0x1C);
	spin_unlock_irq(&pdata->lock);
	
	return ret;
}


static int pmb8876_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct pmb8876rtc_plat_data *pdata = get_pmb8876rtc_plat_data(dev);
	unsigned long sec;

	spin_lock_irq(&pdata->lock);
	sec = readl((void *)pdata->base + 0x1C);
	spin_unlock_irq(&pdata->lock);
	
	rtc_time_to_tm(sec, tm);
	return rtc_valid_tm(tm);
}



static const struct rtc_class_ops pmb8876_rtc_ops = {
	.read_time		= pmb8876_rtc_read_time,
	.set_time       	= pmb8876_rtc_set_time,
};


static int __init pmb8876_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	struct pmb8876rtc_plat_data *pdata;
	struct resource *res;
	int irq, ret;

	/*irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return -ENODEV;*/
	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pdata->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pdata->base))
		return PTR_ERR(pdata->base);
	
	writel(0xAA, (void *)SCU_RTCIF);
	
	writel(0x100, (void *)pdata->base);
	writel(readl((void *)pdata->base + 0x10) | 0x10u, (void *)pdata->base + 0x10);
	
	writel(2, (void *)pdata->base + 0x14);
	writel(0xF000F000, (void *)pdata->base + 0x18);
	writel(0, (void *)pdata->base + 0x20);
	writel(-21075, (void *)pdata->base + 0x2C);
	writel(0, (void *)pdata->base + 0x24);
	writel(0, (void *)pdata->base + 0x28);
	writel( readl((void *)pdata->base + 0x10) | 0x510, (void *)pdata->base + 0x10);
	
	if ( readl((void *)pdata->base + 0x10) & 0x200 ){
	    ret = 3;
	    
	} else {
	    writel(readl((void *)pdata->base + 0x14) | 1, (void *)pdata->base + 0x14);
	    ret = 0;
	}
	
	platform_set_drvdata(pdev, pdata);
	
	spin_lock_init(&pdata->lock);
	//pmb8876_rtc_cmd(pdata->rtcreg, TX4939_RTCCTL_COMMAND_NOP);
	
	/*if (devm_request_irq(&pdev->dev, irq, pmb8876_rtc_interrupt,
			     0, pdev->name, &pdev->dev) < 0)
		return -EBUSY;*/
	rtc = devm_rtc_device_register(&pdev->dev, pdev->name,
				  &pmb8876_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc))
		return PTR_ERR(rtc);
	
	pdata->rtc = rtc;

	return ret;
}

static int __exit pmb8876_rtc_remove(struct platform_device *pdev)
{
	struct pmb8876rtc_plat_data *pdata = platform_get_drvdata(pdev);

	spin_lock_irq(&pdata->lock);
	
	writel(0, (void *)SCU_RTCIF);
	
	spin_unlock_irq(&pdata->lock);
	return 0;
}

static struct platform_driver pmb8876_rtc_driver = {
	.remove		= __exit_p(pmb8876_rtc_remove),
	.driver		= {
		.name		= "pmb8876rtc",
	},
};

module_platform_driver_probe(pmb8876_rtc_driver, pmb8876_rtc_probe);

MODULE_AUTHOR("zvova7890@gmail.com");
MODULE_DESCRIPTION("PMB8876 internal RTC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pmb8876rtc");
