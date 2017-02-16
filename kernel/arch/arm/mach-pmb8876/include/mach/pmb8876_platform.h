
#ifndef __PMB8876_PLATFORM_H__
#define __PMB8876_PLATFORM_H__



/* the base address of IO of PMB8876 */
#define PMB8876_IO_BASE 0xf0000000

/* for present time this value is enough */
#define PMB8876_IO_SIZE (0xff000000 - 0xf0000000)

/* the second watchdog in external place. TODO sleep it */
#define PMB8876_EXT_WATCHDOG    0xf4300118


int pmb8876_serve_watchdog(void);


void pmb8876_init_irq(void);
void pmb8876_init_time(void);




#endif /* __PMB8876_PLATFORM_H__ */
