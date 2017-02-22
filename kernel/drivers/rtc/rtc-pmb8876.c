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
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gfp.h>
#include <mach/pmb8876_platform.h>


#define SCU_RTCIF		0xF4400064
#define SCU_RTC_UNISOLATE	(0xAA)
#define SCU_RTC_ISOLATE 	(0)


#define RTC_CTRL(base)		(((u32)base) + 0x10)
#define RTC_CON(base)		(((u32)base) + 0x14)


#define RTC_CTRL_OUTEN		(1 << 0)	/* external interrupt output enable */
#define RTC_CTRL_INT		(1 << 1)	/* rtc interrupt status */
#define RTC_CTRL_32KEN		(1 << 2) 	/* enable 32k osc */
#define RTC_CTRL_PU32K		(1 << 3) 	/* power up 32k osc */
/*
    Logic Clock Select
	0 - 32 kHz clock operation mode (Asynchronous to microcontroller clock, low power, read only)
	1 - Bus clock operation mode (Synchronous to microcontroller clock, required for register write operation for some registers)
 */
#define RTC_CTRL_CLK_SEL	(1 << 4) 
#define RTC_CTRL_CLRINT 	(1 << 8)	/* clear interrupt bit */
#define RTC_CTRL_BAD		(1 << 9)	/* rtc bad detect bit */
#define RTC_CTRL_CLRBAD 	(1 << 10)	/* clear bad bit */



#define RTC_CON_RUN		(1 << 0)	/* enable rtc */
#define RTC_CON_PRE		(1 << 1)	/* Input Source Pre-Scaler Enable */
#define RTC_CON_ACCPOS		(1 << 15)	/* Register Access Possible */



#define R_RTC_CTRL(base)	readl((void *)RTC_CTRL(base))
#define W_RTC_CTRL(base, x)	writel((x), (void *)RTC_CTRL(base))
#define R_RTC_CON(base) 	readl((void *)RTC_CON(base))
#define W_RTC_CON(base, x)	writel((x), (void *)RTC_CON(base))


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
	
	// set bus clock op mode
	W_RTC_CTRL(pdata->base, R_RTC_CTRL(pdata->base) | RTC_CTRL_CLK_SEL);
	udelay(100);
	
	// unlock write access and stop timer
	W_RTC_CON(pdata->base, (R_RTC_CON(pdata->base) & ~RTC_CON_RUN) | RTC_CON_ACCPOS);
	
	// write new value
	__raw_writel(sec, (void *)pdata->base + 0x1C);
	
	// lock rw access and run timer
	W_RTC_CON(pdata->base, (R_RTC_CON(pdata->base) & ~RTC_CON_ACCPOS) | RTC_CON_RUN);
	
	// set 32k osc mode
	W_RTC_CTRL(pdata->base, R_RTC_CTRL(pdata->base) & ~RTC_CTRL_CLK_SEL );
	
	spin_unlock_irq(&pdata->lock);
	return ret;
}


static int pmb8876_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct pmb8876rtc_plat_data *pdata = get_pmb8876rtc_plat_data(dev);
	unsigned long sec;

	spin_lock_irq(&pdata->lock);
	sec = __raw_readl((void *)pdata->base + 0x1C);
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
	
	// unisolate rtc 
	writel(SCU_RTC_UNISOLATE, (void *)SCU_RTCIF);
	
	writel(0x100, (void *)pdata->base);
	
	// enable 32k osc
	W_RTC_CTRL(pdata->base, R_RTC_CTRL(pdata->base) | RTC_CTRL_PU32K | RTC_CTRL_32KEN );
	
	// unlock write access, setup pre-scaler
	W_RTC_CON(pdata->base, R_RTC_CON(pdata->base) | RTC_CON_ACCPOS | RTC_CON_PRE);
	
	// some unknown magic
	writel(0xF000F000, (void *)pdata->base + 0x18);
	writel(0, (void *)pdata->base + 0x20);
	writel(-21075, (void *)pdata->base + 0x2C);
	writel(0, (void *)pdata->base + 0x24);
	writel(0, (void *)pdata->base + 0x28);
	
	// set bus clock op mode
	W_RTC_CTRL(pdata->base, R_RTC_CTRL(pdata->base) | RTC_CTRL_CLK_SEL | RTC_CTRL_CLRBAD | RTC_CTRL_CLRINT );
	udelay(100);
	
	if ( R_RTC_CTRL(pdata->base) & RTC_CTRL_BAD ){
	    ret = -22;
	    
	} else {
	    // start count
	    W_RTC_CON(pdata->base, (R_RTC_CON(pdata->base) & ~RTC_CON_ACCPOS) | RTC_CON_RUN);
	    ret = 0;
	}
	
	// set 32k osc mode
	W_RTC_CTRL(pdata->base, R_RTC_CTRL(pdata->base) & ~RTC_CTRL_CLK_SEL );
	udelay(100);
	
	platform_set_drvdata(pdev, pdata);
	spin_lock_init(&pdata->lock);
	
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
	writel(SCU_RTC_ISOLATE, (void *)SCU_RTCIF);
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
