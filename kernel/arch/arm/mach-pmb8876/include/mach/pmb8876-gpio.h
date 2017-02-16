#ifndef __PMB8876_GPIO_h
#define __PMB8876_GPIO_h

#define PMB8876_GPIO_BASE		0xF4300020

// shift
#define PMB8876_GPIO_IS			0
#define PMB8876_GPIO_OS			4
#define PMB8876_GPIO_PS			8
#define PMB8876_GPIO_DATA		9
#define PMB8876_GPIO_DIR		10
#define PMB8876_GPIO_PPEN		12
#define PMB8876_GPIO_PDPU		13
#define PMB8876_GPIO_ENAQ		15

// mask
#define PMB8876_GPIO_IS_MASK		7
#define PMB8876_GPIO_OS_MASK		7
#define PMB8876_GPIO_PS_MASK		1
#define PMB8876_GPIO_DATA_MASK		1
#define PMB8876_GPIO_DIR_MASK		1
#define PMB8876_GPIO_PPEN_MASK		1
#define PMB8876_GPIO_PDPU_MASK		3
#define PMB8876_GPIO_ENAQ_MASK		1

// Port Select
#define PMB8876_GPIO_PS_ALT		0
#define PMB8876_GPIO_PS_MANUAL	1

// DIR
#define PMB8876_GPIO_DIR_IN		0
#define PMB8876_GPIO_DIR_OUT	1

// DIR
#define PMB8876_GPIO_DATA_LOW		0
#define PMB8876_GPIO_DATA_HIGH		1

// IS / OS
#define PMB8876_GPIO_NO_ALT		0
#define PMB8876_GPIO_ALT0		1
#define PMB8876_GPIO_ALT1		2
#define PMB8876_GPIO_ALT2		3
#define PMB8876_GPIO_ALT3		4
#define PMB8876_GPIO_ALT4		5
#define PMB8876_GPIO_ALT5		6
#define PMB8876_GPIO_ALT6		7

// PDPU
#define PMB8876_GPIO_PDPU_NONE			0
#define PMB8876_GPIO_PDPU_PULLUP		1
#define PMB8876_GPIO_PDPU_PULLDOWN		2

// PPEN
#define PMB8876_GPIO_PPEN_PUSHPULL		0
#define PMB8876_GPIO_PPEN_OPENDRAIN		1

// ENAQ
#define PMB8876_GPIO_ENAQ_ENAQ			1
#define PMB8876_GPIO_ENAQ_NO_ENAQ		0

// GPIO PIN N => reg
#define PMB8876_GPIO_PIN(n) (PMB8876_GPIO_BASE + n * 4)

#define PMB8876_GPIO(is, os, ps, dir, data, ppen, pdpu, enaq) \
	( \
		(PMB8876_GPIO_ ## is		<< PMB8876_GPIO_IS)		| \
		(PMB8876_GPIO_ ## os		<< PMB8876_GPIO_OS)		| \
		(PMB8876_GPIO_PS_ ## ps		<< PMB8876_GPIO_PS)		| \
		(PMB8876_GPIO_DIR_ ## dir	<< PMB8876_GPIO_DIR)	| \
		(PMB8876_GPIO_DATA_ ## data	<< PMB8876_GPIO_DATA)	| \
		(PMB8876_GPIO_PPEN_ ## ppen	<< PMB8876_GPIO_PPEN)	| \
		(PMB8876_GPIO_PDPU_ ## pdpu	<< PMB8876_GPIO_PDPU)	| \
		(PMB8876_GPIO_ENAQ_ ## enaq	<< PMB8876_GPIO_ENAQ)	  \
	)

// FIXME: нужно найти встроенные аналоги в Linux
#define pmb8876_gpio_reg_set_bit(value, v, shift, mask)			(value) = (((value) & ~(mask << shift)) | ((v & mask) << shift))
#define pmb8876_gpio_reg_get_bit(value, shift, mask)			(((value) >> shift) & mask)

// write
#define pmb8876_gpio_reg_set_is(reg, v)				pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_IS,   PMB8876_GPIO_IS_MASK)
#define pmb8876_gpio_reg_set_os(reg, v)				pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_OS,   PMB8876_GPIO_OS_MASK)
#define pmb8876_gpio_reg_set_ps(reg, v)				pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_PS,   PMB8876_GPIO_PS_MASK)
#define pmb8876_gpio_reg_set_dir(reg, v)			pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_DIR,  PMB8876_GPIO_DIR_MASK)
#define pmb8876_gpio_reg_set_data(reg, v)			pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_DATA, PMB8876_GPIO_DATA_MASK)
#define pmb8876_gpio_reg_set_ppen(reg, v)			pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_PPEN, PMB8876_GPIO_PPEN_MASK)
#define pmb8876_gpio_reg_set_pdpu(reg, v)			pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_PDPU, PMB8876_GPIO_PDPU_MASK)
#define pmb8876_gpio_reg_set_enaq(reg, v)			pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_ENAQ, PMB8876_GPIO_ENAQ_MASK)

// read
#define pmb8876_gpio_reg_get_is(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_IS,   PMB8876_GPIO_IS_MASK)
#define pmb8876_gpio_reg_get_os(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_OS,   PMB8876_GPIO_OS_MASK)
#define pmb8876_gpio_reg_get_ps(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_PS,   PMB8876_GPIO_PS_MASK)
#define pmb8876_gpio_reg_get_dir(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_DIR,  PMB8876_GPIO_DIR_MASK)
#define pmb8876_gpio_reg_get_data(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_DATA, PMB8876_GPIO_DATA_MASK)
#define pmb8876_gpio_reg_get_ppen(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_PPEN, PMB8876_GPIO_PPEN_MASK)
#define pmb8876_gpio_reg_get_pdpu(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_PDPU, PMB8876_GPIO_PDPU_MASK)
#define pmb8876_gpio_reg_get_enaq(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_ENAQ, PMB8876_GPIO_ENAQ_MASK)

// pins
#define GPIO_KP_IN0						0
#define GPIO_KP_IN1						1
#define GPIO_KP_IN2						2
#define GPIO_KP_IN3						3
#define GPIO_KP_IN4						4
#define GPIO_KP_IN5						5
#define GPIO_KP_OUT5_OUT6				6
#define GPIO_KP_OUT0					7
#define GPIO_KP_OUT1					8
#define GPIO_KP_OUT2					9
#define GPIO_KP_OUT3					10
#define GPIO_USART0_RXD					11
#define GPIO_USART0_TXD					12
#define GPIO_USART0_RTS					13
#define GPIO_USART0_CTS					14
#define GPIO_DSPOUT0					15
#define GPIO_USART1_RXD					16
#define GPIO_USART1_TXD					17
#define GPIO_USART1_RTS					18
#define GPIO_USART1_CTS					19
#define GPIO_USB_DPLUS					20
#define GPIO_USB_DMINUS					21
#define GPIO_UNK_22						22
#define GPIO_UNK_23						23
#define GPIO_UNK_24						24
#define GPIO_UNK_25						25
#define GPIO_UNK_26						26
#define GPIO_UNK_27						27
#define GPIO_I2C_SCL					28
#define GPIO_I2C_SDA					29
#define GPIO_UNK_30						30
#define GPIO_UNK_31_I2S2				31
#define GPIO_UNK_32_I2S2				32
#define GPIO_UNK_33_I2S2				33
#define GPIO_UNK_34_I2S2				34
#define GPIO_UNK_35						35
#define GPIO_UNK_36						36
#define GPIO_UNK_37						37
#define GPIO_UNK_38						38
#define GPIO_UNK_39						39
#define GPIO_UNK_40						40
#define GPIO_UNK_41						41
#define GPIO_UNK_42						42
#define GPIO_LED_FL_OFF					43
#define GPIO_TOUT1_PM_CHARGE_UC			44
#define GPIO_UNK_45						45
#define GPIO_UNK_46						46
#define GPIO_UNK_47						47
#define GPIO_UNK_48						48
#define GPIO_UNK_49						49
#define GPIO_TOUT7_PM_RF2_EN			50
#define GPIO_UNK_51						51
#define GPIO_TOUT9_I2C_2_DAT			52
#define GPIO_TOUT10_SERIAL_EN			53
#define GPIO_TOUT11_I2C_2_CLK			54
#define GPIO_UNK_55						55
#define GPIO_RF_STR0					56
#define GPIO_RF_STR1_PM_RINGIN			57
#define GPIO_MMC_VCC_EN					58
#define GPIO_RF_CLK						59
#define GPIO_UNK_60						60
#define GPIO_UNK_61						61
#define GPIO_DSPOUT1_PM_WADOG			62
#define GPIO_LED_FL_EN					63
#define GPIO_UNK_64						64
#define GPIO_UNK_65						65
#define GPIO_UNK_66						66
#define GPIO_UNK_67						67
#define GPIO_UNK_68						68
#define GPIO_UNK_69						69
#define GPIO_UNK_70						70
#define GPIO_UNK_71						71
#define GPIO_UNK_72						72
#define GPIO_UNK_73						73
#define GPIO_UNK_74						74
#define GPIO_UNK_75						75
#define GPIO_UNK_76						76
#define GPIO_MMC_CD						77
#define GPIO_CIF_D0						78
#define GPIO_CIF_D1						79
#define GPIO_CIF_D2						80
#define GPIO_CIF_D3						81
#define GPIO_CIF_D4						82
#define GPIO_CIF_D5						83
#define GPIO_CIF_D6						84
#define GPIO_CIF_D7						85
#define GPIO_CIF_PCLK					86
#define GPIO_CIF_HSYNC					87
#define GPIO_CIF_VSYNC					88
#define GPIO_CLKOUT2					89
#define GPIO_UNK_90						90
#define GPIO_UNK_91						91
#define GPIO_UNK_92						92
#define GPIO_UNK_93						93
#define GPIO_UNK_94						94
#define GPIO_UNK_95						95
#define GPIO_UNK_96						96
#define GPIO_UNK_97						97
#define GPIO_UNK_98						98
#define GPIO_UNK_99						99
#define GPIO_UNK_100					100
#define GPIO_UNK_101					101
#define GPIO_UNK_102					102
#define GPIO_UNK_103					103
#define GPIO_MMC_CMD					104
#define GPIO_MMC_DAT					105
#define GPIO_MMC_CLK					106
#define GPIO_UNK_107					107
#define GPIO_UNK_108					108
#define GPIO_UNK_109					109
#define GPIO_UNK_110					110
#define GPIO_UNK_111					111
#define GPIO_I2S1_CLK1					112
#define GPIO_CIF_PD						113
#define GPIO_UNK_116					116

#endif //__PMB8876_GPIO_h
