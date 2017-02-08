/*
 * Serial port stubs for kernel decompress status messages
 *
 * Initially based on:
 * arch/arm/plat-omap/include/mach/uncompress.h
 *
 * Original copyrights follow.
 *
 * Copyright (C) 2000 RidgeRun, Inc.
 * Author: Greg Lonnon <glonnon@ridgerun.com>
 *
 * Rewritten by:
 * Author: <source@mvista.com>
 * 2004 (c) MontaVista Software, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/types.h>
#include <linux/serial_reg.h>

#include <asm/mach-types.h>



static inline void putc(char c)
{
}

static inline void flush(void)
{

}

static inline void set_uart_info(u32 phys)
{
}


static inline void __arch_decomp_setup(unsigned long arch_id)
{

}

#define arch_decomp_setup()	__arch_decomp_setup(arch_id)
