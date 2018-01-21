#include <linux/module.h>
#include <generated/mach-types.h>

#include <asm/irq.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <linux/platform_device.h>
#include <asm/mach-types.h>
#include <asm/setup.h>
#include <asm/tlbflush.h>
#include <asm/pgtable.h>

#include <mach/pmb8876_platform.h>


/*
 *
    [SIEMENS:EL71:45]
    Name = Siemens EL71 fw45
    BootROMaddress = 0x400000;
    BootROMsize = 0x100000;
    FlashAddress = 0xa0000000;
    FlashSize = 0x04000000;
    IntRAM1address = 0x0;
    IntRAM1size = 0x4000;
    IntRAM2address = 0x80000;
    IntRAM2size = 0x18000;
    ExtRAMaddress = 0xa8000000;
    ExtRAMsize = 0x01000000;
    VMalloc1address=0xaA000000
    VMalloc1size=0x01000000
    VMalloc2address=0xaB000000
    VMalloc2size=0x01000000
    VMalloc3address=0xaC000000
    VMalloc3size=0x01000000
    VMalloc4address=0xaD000000
    VMalloc4size=0x01000000
    MiscSpaceaddress=0xc0000000
    MiscSpacesize=0x00100000
    IOaddress = 0xf0000000;
    IOsize =    0x10000000;
    UseRAM = 0x89000;
    MallocAddress = 0xA0092F51;
    MallocPages = 300;
*/


static void pmb8876_switch_watchdog(void)
{
    unsigned int r2 = (*(int *)PMB8876_EXT_WATCHDOG);
    unsigned int r0 = r2 << 22;
    r0 = r0 >> 31;
    r0 = ~r0;
    r0 = r0 & 1;
    r0 = r0 << 9;

    r2 = r2 & ~0x200;

    (*(int *)PMB8876_EXT_WATCHDOG) = r0 | r2;
}



/*
 * We need to map the PMB8876 0xf0 -> 0xff IO space area
 */
static struct map_desc pmb8876_io_desc[] __initdata __maybe_unused = {
    {
        .virtual	= PMB8876_IO_BASE,
        .pfn		= __phys_to_pfn(PMB8876_IO_BASE),
        .length		= PMB8876_IO_SIZE,
        .type		= MT_DEVICE
    }
};

static void __init pmb8876_map_io(void)
{
    iotable_init(pmb8876_io_desc, ARRAY_SIZE(pmb8876_io_desc));
}


static void __init pmb8876_early_init(void)
{
    pmb8876_cpu_clock_init();
}


static void __init pmb8876_board_init(void)
{
    pr_info("Initializing PMB8876 board...\n");
}


/*
 * Platform specific handler that read's current IRQ number and call handle_IRQ
 */
asmlinkage void pmb8876_handle_irq(struct pt_regs *regs)
{
    int irqn = readl((void *)0xF280001C);
    
    if( irqn > 7 && irqn != 0x77 && irqn != 148 ) {
        pr_info("irq %d fired!\n", irqn);
    }
    
    handle_IRQ(irqn, regs);
}


/*
 * FIXME
 */
static void pmb8876_reboot(enum reboot_mode mode, const char *cmd)
{
	pr_info("%s()\n", __func__);
    
	pmb8876_switch_watchdog();
	pmb8876_switch_watchdog();
	while(1);
}


MACHINE_START(PMB8876, "PMB8876")
    .map_io         = pmb8876_map_io,
    .init_early     = pmb8876_early_init,
    .init_machine   = pmb8876_board_init,
    .init_irq		= pmb8876_init_irq,
    .handle_irq     = pmb8876_handle_irq,
    .init_time      = pmb8876_init_time,
    .restart        = pmb8876_reboot,
    .nr_irqs		= 0x78,
MACHINE_END
