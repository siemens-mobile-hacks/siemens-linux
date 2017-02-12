#ifndef __PMB8876_I2C_h
#define __PMB8876_I2C_h

#define PMB8876_I2C_D1601AA		0x31 // Dialog/Twigo
#define PMB8876_I2C_TEA5761UK	0x10 // FM Radio

// D1601AA regs
#define D1601AA_LIGHT_PWM1		0x12 /* display backlight level 0x00...0x64 */
#define D1601AA_LIGHT_PWM2		0x13 /* keyboard backlight level 0x00...0x64 */
#define D1601AA_LED_CONTROL		0x14 /* led control */
#define D1601AA_VIBRA			0x47 /* vibra level 0x00...0x64 */

// D1601AA_LED_CONTROL values
#define D1601AA_LED1_EN			(1 << 1) // <-- на E71 тут SLI led, на EL71 не подключен
#define D1601AA_LED2_EN			(1 << 2) // <-- Сюда подключен EXTBOOST_EN, что бы работала вспышка и подсветки нужно включить его
#define D1601AA_LIGHT_PWM1_EN	(1 << 3) // <-- Подсветка дисплея
#define D1601AA_LIGHT_PWM2_EN	(1 << 4) // <-- Подсветка клавиатуры

#endif //__PMB8876_I2C_h
