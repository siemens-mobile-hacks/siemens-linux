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

#define EBU_SDRMCON0 0xF0000050
#define EBU_SDRMCON1 0xF0000058


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


/*
 
 12  MHz: 2, 2, 1, 6
 18  MHz: 2, 2, 1, 4
 
 26  MHz: 1, 1, 1, 0
 52  MHz: 2, 2, 1, 1
 78  MHz: 2, 1, 1, 0
 104 MHz: 3, 1, 1, 0
 156 MHz: 2, 2, 1, 0
 208 MHz: 3, 2, 1, 0
 260 MHz: 4, 2, 1, 0
 312 MHz: 5, 2, 1, 0
 
 */



struct PMB8876_Clocks {
    int rate;
    int mul1, mul2;
    int div1, div2;
};



/* current cpu rate */
static int pmb8876_current_cpu_clock = 12000;

/* cpu clock div/mul values list */
struct PMB8876_Clocks pmb8876_clocks[] = {
    {
        .rate = 12000,
        .mul1 = 2,
        .mul2 = 2,
        .div1 = 1,
        .div2 = 6
    },
    
    {
        .rate = 18000,
        .mul1 = 2,
        .mul2 = 2,
        .div1 = 1,
        .div2 = 4
    },
    
    {
        .rate = 26000,
        .mul1 = 1,
        .mul2 = 1,
        .div1 = 1,
        .div2 = 0
    },
    
    {
        .rate = 52000,
        .mul1 = 2,
        .mul2 = 2,
        .div1 = 1,
        .div2 = 1
    },
    
    {
        .rate = 78000,
        .mul1 = 2,
        .mul2 = 1,
        .div1 = 1,
        .div2 = 0
    },
    
    {
        .rate = 104000,
        .mul1 = 3,
        .mul2 = 1,
        .div1 = 1,
        .div2 = 0
    },
    
    {
        .rate = 156000,
        .mul1 = 2,
        .mul2 = 2,
        .div1 = 1,
        .div2 = 0
    },
    
    {
        .rate = 208000,
        .mul1 = 3,
        .mul2 = 2,
        .div1 = 1,
        .div2 = 0
    },
    
    {
        .rate = 260000,
        .mul1 = 4,
        .mul2 = 2,
        .div1 = 1,
        .div2 = 0
    },
    
    {
        .rate = 312000,
        .mul1 = 5,
        .mul2 = 2,
        .div1 = 1,
        .div2 = 0
    },
};


static unsigned int init_pll(void)
{
    /*IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFFFFF8) | 3 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFFFF87) | 8 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFFF8FF) );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFF87FF) | 0x1800 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFF8FFFF) | 0x40000 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFF87FFFF) | 0x80000 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xF8FFFFFF) | 0x1000000 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0x87FFFFFF) | 0x10000000 );*/
    
    
    //( 0 << 12 ) - делитель, 3х битный
    IOPLL_CON0_SET( 0x11200800 | (0 << 16) | (0 << 12) ); 

    IOPLL_ICR_SET( IOPLL_ICR() & 0xFFFFEFFF );
    IOPLL_OSC_SET( IOPLL_OSC() & 0xF0FFFFFF );
    IOPLL_OSC_SET( (IOPLL_OSC() & 0xFFC0FFFF) | 0x30000 );
    IOPLL_OSC_SET( IOPLL_OSC() | 0x1000u );
    IOPLL_OSC_SET( IOPLL_OSC() | 0x100u );
    IOPLL_OSC_SET( IOPLL_OSC() | 0x10u );
    IOPLL_OSC_SET( IOPLL_OSC() | 1u );

    while ( !(IOPLL_STAT() & 0x2000) );

    IOPLL_CON2_SET( (IOPLL_CON2() & 0xFFFF3FFF) | 0x8000 );
    writel( (readl((void *)0xF45000B4) & 0xFCFFFFFF) | 0x1000000, (void *)0xF45000B4 );
    
    return 0;
}


static void EBU_wtf_clock_reinit_2(void)
{
    writel( readl((void *)0xF4400044) | 1u, (void *)0xF4400044 );
    while ( !(readl((void *)0xF4400044) & 0x10) );
}


/*
 12  MHz: 2, 2, 1, 6
 18  MHz: 2, 2, 1, 4
 
 26  MHz: 1, 1, 1, 0
 52  MHz: 2, 2, 1, 1
 78  MHz: 2, 1, 1, 0
 104 MHz: 3, 1, 1, 0
 156 MHz: 2, 2, 1, 0
 208 MHz: 3, 2, 1, 0
 260 MHz: 4, 2, 1, 0
 312 MHz: 5, 2, 1, 0
 */

static unsigned int pmb8876_pll_reclock(char mul1, char mul2, char div1, char div2)
{
    //( 1 << 12 ) - делитель, 3х битный
    IOPLL_CON0_SET( 0x11200800 | (0 << 16) | (div2 << 12) ); 
    
    // (5 << 16) - множитель 1 - 5
    // (5 << 8) - тоже какая-то хуитка, похожая на множитель
    writel((mul1 << 16) | (0 << 12) | (5 << 8) | 5, (void *)PLL_OSC);
    
    //printk(" -> osc: %X\n", readl((void *)PLL_OSC));

    // (1 << 5) - PLL_CONNECT
    // (1 << 8) - делитель, очень стрёмный, при увеличении вроде тупее работает, но богомипсов одинаково
    writel((0x10 << 24) | (7 << 13) | (0 << 12) | (div1 << 8) | (1 << 5) | 0x7, (void *)PLL_CON2);
    //printk(" -> con2: %X\n", readl((void *)PLL_CON2));
    
    EBU_wtf_clock_reinit_2();
    
    // (1 << (20+0)) - иножитель
    // (1 << (16+0)) - хрень какая-то
    // (1 << 1) - если установлен, нужно пересчитывать CLC периферии
    writel((1 << (20+mul2)) | (0 << 16) | 0, (void *)PLL_CON1);
    //printk(" -> con1: %X\n", readl((void *)PLL_CON1));
    
    writel(readl((void *)0xF45000B4) | 0x310000, (void *)0xF45000B4);
    //printk(" -> b4: %X\n", readl((void *)0xF45000B4));
    
    //printk("con0: %X\n", IOPLL_CON0());
    
    writel( readl((void *)0xF4400040) | 1u, (void *)0xF4400040);
    while ( !(readl((void *)0xF4400040) & 0x10) );
    
    IOEBU_CON_SET( IOEBU_CON() | 0x4000000u );
    writel(0, (void *)0xF4400040);
    
    
    writel( readl((void *)0xF4400040) | 1u, (void *)0xF4400040 );
    while ( !(readl((void *)0xF4400040) & 0x10) );
    
    IOEBU_CON_SET( IOEBU_CON() & 0xFBFFFFFF );
    writel(0, (void *)0xF4400040);
    
    return 0;
}


int pmb8876_get_cpu_rate(void)
{
    return pmb8876_current_cpu_clock;
}


int pmb8876_set_cpu_rate(int rate)
{
    int  i = 0;
    
    for( i = 0; i < ARRAY_SIZE(pmb8876_clocks); i++ ) {
        if( pmb8876_clocks[i].rate == rate ) {
            pmb8876_pll_reclock(pmb8876_clocks[i].mul1, pmb8876_clocks[i].mul2,
                    pmb8876_clocks[i].div1, pmb8876_clocks[i].div2);
            pmb8876_current_cpu_clock = rate;
            return 0;
        }
    }
    
    return -EINVAL;
}


static void EBU_init(void)
{
    //(16 - 3) - Row cycle time counter
    //(14 - 2) - Row to column delay counter
    //(10 - 2) - Row precharge time counter
    
    writel( 0xD02070 | (7 << 16) | (2 << 14) | (2 << 10), (void *)EBU_SDRMCON0 );
    writel( 0xD02070 | (7 << 16) | (2 << 14) | (2 << 10), (void *)EBU_SDRMCON1 );
    
    //(4 - 3) - cas latency
    // 0x33
    writel( (3 << 4) | 3, (void *)0xF0000060);
    writel( (3 << 4) | 3, (void *)0xF0000068);
}


void pmb8876_cpu_clock_init(void)
{
    volatile int i = 0;
    
    pr_info("Initializing PLL...\n");
    EBU_init();
    for( ; i < 1000; i++);
    
    init_pll();
    
    pmb8876_set_cpu_rate(312000);
    pr_info("Switched to %d MHz\n", pmb8876_get_cpu_rate() / 1000);
}
