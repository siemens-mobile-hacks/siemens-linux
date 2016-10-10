
.macro	addruart,rd,rx
	
.endm

.macro	senduart,rd,rx
	mov \rx, #0xf1000000
	and \rd, \rd, #0xFF
	str \rd, [\rx, #0x20]
1001:
	ldr \rd, [\rx, #0x68]
	ands \rd, \rd, #0x02
	beq 1001b
	
	ldr \rd, [\rx, #0x70] 
	orr \rd, \rd, #2
	str \rd, [\rx, #0x70]
.endm

.macro	busyuart,rd,rx
	
.endm

.macro	waituart,rd,rx
	
.endm


