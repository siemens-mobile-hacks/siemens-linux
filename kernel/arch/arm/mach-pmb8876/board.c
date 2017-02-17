#include <linux/module.h>

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

#define PLL_OSC 0xF45000A0
#define PLL_CON0 0xF45000A4
#define PLL_CON1 0xF45000A8
#define PLL_CON2 0xF45000AC
#define PLL_STAT 0xF45000B0
#define PLL_ICR 0xF45000CC

#define SCU_BASE 0xF4400000
#define SCU_RST_CTRL_ST 0xF4400018
#define SCU_WDTCON0 0xF4400024
#define SCU_WDTCON1 0xF4400028
#define SCU_PLLCLC 0xF4400044
#define SCU_CHIPID 0xF4400060
#define SCU_RTCIF 0xF4400064
#define SCU_CHIPID 0xF4400060
#define SCU_ROMAMCR 0xF440007C

#define EBU_CLC 0xF0000000
#define EBU_ID 0xF0000008
#define EBU_CON 0xF0000010
#define EBU_BFCON 0xF0000020



#define IOPLL_CON0()		readl((void *)PLL_CON0)
#define IOPLL_CON0_SET(x)	writel(x, (void *)PLL_CON0)

#define IOPLL_CON1()		readl((void *)PLL_CON1)
#define IOPLL_CON1_SET(x)	writel(x, (void *)PLL_CON1)

#define IOPLL_CON2()		readl((void *)PLL_CON2)
#define IOPLL_CON2_SET(x)	writel(x, (void *)PLL_CON2)

#define IOPLL_ICR()		readl((void *)PLL_ICR)
#define IOPLL_ICR_SET(x)	writel(x, (void *)PLL_ICR)

#define IOPLL_OSC()		readl((void *)PLL_OSC)
#define IOPLL_OSC_SET(x)	writel(x, (void *)PLL_OSC)

#define IOPLL_STAT()		readl((void *)PLL_STAT)

#define IOSCU_PLLCLC()		readl((void *)SCU_PLLCLC)
#define IOSCU_PLLCLC_SET(x)	writel(x, (void *)SCU_PLLCLC)

#define IOEBU_CON()		readl((void *)EBU_CON)
#define IOEBU_CON_SET(x)	writel(x, (void *)EBU_CON)


unsigned int init_pll(void)
{
    unsigned int result; // r0@3

    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFFFFF8) | 3 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFFFF87) | 8 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFFF8FF) );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFF87FF) | 0x1800 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFF8FFFF) | 0x40000 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFF87FFFF) | 0x80000 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xF8FFFFFF) | 0x1000000 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0x87FFFFFF) | 0x10000000 );

    IOPLL_ICR_SET( IOPLL_ICR() & 0xFFFFEFFF );
    IOPLL_OSC_SET( IOPLL_OSC() & 0xF0FFFFFF );
    IOPLL_OSC_SET( (IOPLL_OSC() & 0xFFC0FFFF) | 0x30000 );
    IOPLL_OSC_SET( IOPLL_OSC() | 0x1000u );
    IOPLL_OSC_SET( IOPLL_OSC() | 0x100u );
    IOPLL_OSC_SET( IOPLL_OSC() | 0x10u );
    IOPLL_OSC_SET( IOPLL_OSC() | 1u );

    while ( !(IOPLL_STAT() & 0x2000) );

    IOPLL_CON2_SET( (IOPLL_CON2() & 0xFFFF3FFF) | 0x8000 );
    result = (readl((void *)0xF45000B4) & 0xFCFFFFFF) | 0x1000000;
    writel( (readl((void *)0xF45000B4) & 0xFCFFFFFF) | 0x1000000, (void *)0xF45000B4 );
    return result;
}


unsigned int pll_reclock_104(void)
{
    unsigned int pll_con2, pll_con1 = IOPLL_CON1() & 0xFF8FFFFF;
    
    IOPLL_CON1_SET( pll_con1 | 0x200000 );
    
    IOSCU_PLLCLC_SET( IOSCU_PLLCLC() & 0xFFFFFFFE );
    while ( !(IOSCU_PLLCLC() & 0x10) );
    
    pll_con2 = IOPLL_CON2() & 0x1000;
    pll_con2 &= 0x370;
    pll_con2 |= 0x70;
    IOPLL_CON2_SET( pll_con2 );
    
    IOPLL_OSC_SET( IOPLL_OSC() & ~0x404 );
    
    writel( readl((void *)0xF4400040) | 1u, (void *)0xF4400040);
    while ( !(readl((void *)0xF4400040) & 0x10) );
    
    IOEBU_CON_SET( IOEBU_CON() | 0x4000000u );
    writel(0, (void *)0xF4400040);
    
    
    writel( readl((void *)0xF4400040) | 1u, (void *)0xF4400040 );
    while ( !(readl((void *)0xF4400040) & 0x10) );
    
    IOEBU_CON_SET( IOEBU_CON() & 0xFBFFFFFF );
    writel(0, (void *)0xF4400040);
    
    return SCU_BASE;
}


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
    pr_info(" %s\n", __func__);
    init_pll();
    pll_reclock_104();
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
    
    /*if( irqn != 0x77 ) {
        pr_info("irq %d fired!\n", irqn);
    }*/
    
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
