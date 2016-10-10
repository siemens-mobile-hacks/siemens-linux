#include <linux/module.h>

#include <asm/irq.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <linux/platform_device.h>
#include <asm/mach-types.h>
#include <asm/setup.h>

static void __init board_init(void) {
	printk(KERN_INFO "%s(): HELLO BLJAD\n", __func__);
}
static void __init init_irq(void) {
	
}

MACHINE_START(PMB8876, "PMB8876")
	.init_machine   = board_init,
	.init_irq		= init_irq,
	.nr_irqs		= 0
MACHINE_END