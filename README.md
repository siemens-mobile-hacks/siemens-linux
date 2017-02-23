# siemens-linux
Linux for Siemens EL71 (pmb8876)

# Done at this moment:
1. MMU support
2. PLL re-clocking for 104 MHz
3. EBU unlock 16m of ram(done with bootloader)
4. UART async driver that working with IRQ
5. GSM Timer for sched, STM timer that runs at 26MHz for hi-resolution timing support. Also work with tickless kernel configuration.
6. WatchDog timer support
7. clocksource driver for high accuracy time support
8. GPIO support
9. Soft i2c support via gpiolib
10. NOR flash R/W support(done by linux builtin driver that provide an MTD device)
11. MMC sdio via AMBA driver(currently without DMA support)
12. RTC support
