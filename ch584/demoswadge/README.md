# Tempalte repo for demo swadge

 * CPU: CH584M (C42381470)
   * 78MHz, RISC-V, IMBC
   * 504kB Flash
   * 96kB RAM
   * 2.4GHz support
   * USBFS (Could be USBHS if 585M)
   * 40 GPIOs
 * Single AA Battery, BH-AA-A1AJ020
 * Boost supply
   * C5345596
   * 2.2V VDD
 * OLED or LCD (Needs more discussion) 
 * Silicone Buttons	
 * LEDs (Recommend no LEDs) LC8805C
 * SMALLER PCB (Recommend 35x60)
 * Add a case (Need to explore)

### Costs (@1000)

| Component | Cost | Energy (Active, 100% RX) |
| --- | --- | --- |
| CPU | $0.559 | 10uA - 1mA - 7mA (RX) - peak 12mA (TX) |
| Boost Supply | $0.046 | 10uA |
| Battery Holder | $0.22 | - |
| LCD | $2.00 to $3.00 (depending) | - |
| Silicone Buttons | $0.16 | - |
| LEDs 4x | $0.18 | 2mA to 62mA |
| PCB | $0.21 | - |

Silicone button candidates:
 * ![PS3 controller](https://www.alibaba.com/product-detail/Conductive-Silicone-Button-Pad-for-GB_1601404877559.html)
 * ![Various Controllers](https://www.alibaba.com/product-detail/2026R-Silicone-Rubber-Pad-for-Ps5_1601622066102.html)

LEDs:
 * LC8805C

PCB:
 * Assumes PCBWay, 35x60, 2L, Lead-free HASL
