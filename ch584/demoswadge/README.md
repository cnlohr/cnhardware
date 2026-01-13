# cnlohr's ideal mini swadge

## Goals
 * Swadges have lost their novelty because they're all the same
 * Swadges are too heavy and big.  Smaller. Lighter.
 * Swadges are expensive, what if we make them much cheaper? Can we make more?
 * Keep as much connectivity/features as are visible.  We have to pick and choose.
   * USB ($0.05)
   * Buttons ($0.10)
   * Audio Out ($0.14)
   * Audio In ($0.18)
   * LEDs ($0.35)
   * IMU ($0.55)
   * Compatibility with ESP32-S2 wireless ($1.80)
   * Display ($1.20)
   * Color Display ($2.80)
 * Needs to be more enclosed.

## Spec

 * Single AA Battery, BH-AA-A1AJ020
 * CPU: CH584M (C42381470)
   * 78MHz, RISC-V, IMBC
   * 504kB Flash (Optional +512kB in CH584X)
   * 96kB RAM
   * 2.4GHz (BLE-only, no wifi) support
   * NFC (13.56 MHz RFID)
   * USBFS (Could be USBHS if 585M)
   * 40 GPIOs
   * Processor current is:
     * 5uA Standby (with RTC)
     * 2mA Operating, no radio
     * 7mA (RX)
     * 12mA (Peak TX)
 * Boost supply
   * C5345596, 2.2V VDD, CH584 can run at low voltage.
   * LDO for USB operation to 3.3VDD.
 * OLED or LCD
   * Consider OLED because lower power
   * Consider monochrome OLED for cost (Could 2bit greyscale)
   * Consider LCD but higher power, for color.
 * Silicone Buttons	
 * LEDs (Recommend no LEDs) LC8805C
 * SMALLER PCB (Recommend 35x60)
 * Add a case (not quoted) or triple-stack PCB
 * Recommend no speaker, but headphone jack. Digital volume control.
 * Consider soft-power-switch-only to enable RTC
 * USB (For programmability or other features)

### Costs (@1000)

| Component | Cost | Energy (Active, 100% RX) |
| --- | --- | --- |
| CPU | $0.56 | 7mA |
| Boost x2, LDO | $0.11 | 10-20uA |
| Battery Holder | $0.22 | - |
| LCD/OLED | $2.00-$3.00 | 9-100mA |
| Silicone Buttons | $0.06-$0.35 | - |
| PCB | $0.21-$0.63 | - |
| Misc Discretes | $0.15 | - |
| USB (SOUHAN connector) | $0.05 | - |
| Mic (GMI4013S-2C56DB) | $0.14 | - |
| Headphones PJ-307C | $0.04 | - |
| Crystals (assume internal RTC) | $0.04 | - |
| --- | Optional | --- |
| LEDs 4x | $0.18 | 2mA to 62mA |
| LSM6DS3TR | $0.66 | 450uA (Active mode) |

Silicone button candidates:
 * ![PS3 controller](https://www.alibaba.com/product-detail/Conductive-Silicone-Button-Pad-for-GB_1601404877559.html)
 * ![Various Controllers](https://www.alibaba.com/product-detail/2026R-Silicone-Rubber-Pad-for-Ps5_1601622066102.html)
 * DonGuan Deson Insulated MAterial Co., LTD 10x6x9 Clear, 30k, $0.017/ea

LEDs:
 * LC8805C

PCB:
 * Assumes PCBWay, 35x60, 2L, Lead-free HASL
 * If no case, need 2 extra PCBs to sandwich buttons and OLED/LCD panel.

## Software Recommendations

 * Recommend CH32FUN
 * Recommend compressed loading for game mode.
 * Common funcitons remain in FLASH.
 * Each game has 100% of RAM to itself.
 * Assets **may** stay in FLASH or be loaded into RAM.

