#include <linux/module.h>

#include <asm/irq.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <linux/platform_device.h>
#include <asm/mach-types.h>
#include <asm/setup.h>
#include <asm/tlbflush.h>

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




#define EBU_ADDRSEL0 0xF0000080
#define EBU_ADDRSEL1 0xF0000088
#define EBU_ADDRSEL2 0xF0000090
#define EBU_ADDRSEL3 0xF0000098
#define EBU_ADDRSEL4 0xF00000A0
#define EBU_ADDRSEL5 0xF00000A8
#define EBU_ADDRSEL6 0xF00000B0
#define EBU_BUSCON0 0xF00000C0
#define EBU_BUSCON1 0xF00000C8
#define EBU_BUSCON2 0xF00000D0
#define EBU_BUSCON3 0xF00000D8
#define EBU_BUSCON4 0xF00000E0
#define EBU_BUSCON5 0xF00000E8
#define EBU_BUSCON6 0xF00000F0
#define EBU_SDRMREF0 0xF0000040
#define EBU_SDRMOD0 0xF0000060
#define SCU_WDTCON0 0xF4400024
#define SCU_WDTCON1 0xF4400028
#define SCU_ROMAMCR 0xF440007C


/* this variable is used in head.S (saving r12 register from chaos-boot) */
unsigned long last_watchdog_time = 0;


static void pmb8876_switch_watchdog(void)
{
    unsigned int r2 = (*(int *)SIEMENS_EL71_EXT_WATCHDOG);
    unsigned int r0 = r2 << 22;
    r0 = r0 >> 31;
    r0 = ~r0;
    r0 = r0 & 1;
    r0 = r0 << 9;

    r2 = r2 & ~0x200;

    (*(int *)SIEMENS_EL71_EXT_WATCHDOG) = r0 | r2;
}


int pmb8876_serve_watchdog(void)
{
    unsigned int now = (*(int *)0xf4b00020);

    if (now - last_watchdog_time < 0x200)
        return 0;

    pmb8876_switch_watchdog();

    last_watchdog_time = now;
    return 1;
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



static void init_sdram(void) {
    
    writel(0xA8000041, (void *)EBU_ADDRSEL1);
    writel(0x30720200, (void *)EBU_BUSCON1);
    writel(6, (void *)EBU_ADDRSEL1);
    writel(0x891C70, (void *)EBU_SDRMREF0);
    writel(0x23, (void *)EBU_SDRMOD0);
    writel(0xA0000011, (void *)EBU_ADDRSEL0);
    writel(0xA0000011, (void *)EBU_ADDRSEL4);
    
	writel(0x00522600, (void *)EBU_BUSCON0);
    writel(0x00522600, (void *)EBU_BUSCON4);
}


void disable_first_whatchdog(void) {
    
    int amcr = readl((void *)SCU_ROMAMCR);
	writel( amcr &= ~1, (void *)SCU_ROMAMCR );
	writel(0x8, (void *)SCU_WDTCON1);
}


void set_einit(char flag) {
	
	// ldr r3, =0xf4400000
	// ldr r1, [r3, #0x24] ;SCU_WDTCON0
	unsigned int wdc0 = readl((void *)SCU_WDTCON0);
	
	//  bic r1, r1, #0x0e
	//  orr r1, r1, #0xf0
	wdc0 &= ~0x0E;
	wdc0 |= 0xf0;
	
	// ldr r2, [r3, #0x28] ;SCU_WDTCON1
	// and r2, r2, #0x0c
	unsigned int wdc1 = readl((void *)SCU_WDTCON1);
	wdc1 &= 0x0c;
	
	// orr r1, r1, r2
	// str r1, [r3, #0x24] ;SCU_WDTCON0
	wdc0 |= wdc1; 
	writel(wdc0, (void *)SCU_WDTCON0);
	
	// bic r1, r1, #0x0d
	// orr r1, r1, #2
	// orr r0, r0, r1
	wdc0 &= ~0x0d;
	wdc0 |= 2;
	wdc0 |= flag;
	writel(wdc0, (void *)SCU_WDTCON0);
}


static void __init pmb8876_board_init(void)
{
    pr_info("%s(): HELLO BLJAD (.)(.)\n", __func__);
    
    /**(unsigned int *)0xF6400048 = 0x100;
    *(unsigned int *)EBU_BUSCON0 & ~0xE000u | 0x2000;*/
    
    /*init_sdram();
    set_einit(0);
    disable_first_whatchdog();
    set_einit(1);*/
}


/*
 * Platform specific handler that read's current IRQ number and call handle_IRQ
 */
asmlinkage void pmb8876_handle_irq(struct pt_regs *regs)
{
    int irqn = readl((void *)0xF280001C);
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
}


MACHINE_START(PMB8876, "PMB8876")
    .map_io         = pmb8876_map_io,
    .init_machine   = pmb8876_board_init,
    .init_irq		= pmb8876_init_irq,
    .handle_irq     = pmb8876_handle_irq,
    .init_time      = pmb8876_init_time,
    .restart        = pmb8876_reboot,
    .nr_irqs		= 0x78,
MACHINE_END
