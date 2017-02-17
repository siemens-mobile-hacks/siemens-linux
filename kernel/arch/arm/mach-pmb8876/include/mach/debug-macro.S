
/*
 * 
 */


.macro	addruart, rp, rv, tmp
#ifdef CONFIG_DEBUG_LL_PMB8876
	/* the physical and virtual address of IO is same */
	mov \rp, #0xf1000000
	mov \rv, #0xf1000000
#endif
.endm

.macro	senduart,rd,rx
#ifdef CONFIG_DEBUG_LL_PMB8876
	and \rd, \rd, #0xFF
	str \rd, [\rx, #0x20]
#endif
.endm

.macro  busyuart,rd,rx
#ifdef CONFIG_DEBUG_LL_PMB8876
1002:
	ldr \rd, [\rx, #0x68]
	ands \rd, \rd, #0x02
	beq 1002b
		
	ldr \rd, [\rx, #0x70] 
	orr \rd, \rd, #2
	str \rd, [\rx, #0x70]
#endif
.endm

.macro	waituart,rd,rx

.endm


