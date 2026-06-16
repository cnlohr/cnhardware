# 




# Methodology


Test program:
```c
	SystemInit();
	Delay_Ms( 1800 );
	funGpioInitAll(); // Enable GPIOs
	GPIOC->CFGLR = 0x11111111;
	GPIOD->CFGLR = 0x44441144; // PD2..3 outputs
	while(1)
	{
		GPIOC->OUTDR = LEDMASK;
		GPIOD->OUTDR = (LEDMASK>>8)<<2;
		ADD_N_NOPS( 15 );
		GPIOC->OUTDR = 0x00;
		GPIOD->OUTDR = 0b0000;
		ADD_N_NOPS( 30 );  (or 300 for IR, etc.)
	}
```

Target wattage: ~1W (+/- 50%)

Settings on SpectraScan PR670:
ND1000 Filter, using 75mm 1:2.7
6ms integration time.
Focus on board

**Radiance values are approximate, and will vary by power, DO NOT USE FOR ABSOLUTE TERMS**



# LEDs

### 490-500 Ice Blue
Board A, top1, .66W (4.94V, 0.133A)
490-500, 3W Aixinde, "Ice Blue", 3W3535H490IB

### 450-460 Royal Blue
Board A, top2, .73W (4.94V, 0.149A)
450-460, 3W Aixinde, "Royal Blue", 3W3535XSJ450RB

### 520-530 Green
Board A, top3, .579W (4.95V, 0.116A)
520-530, 3W Aixinde, "Green" 3W3535XSJ520G / 3W3535XSJ5205?
1.57 W/sr/m^2, 516nm

### IR730
Board A, top4, LOW POWER, 2.27W (4.8V, 0.475A)
730nm, 3W Aixinde, "IR730" 3W3535Y730IR
3 W/sr/m^2, 730nm

### Deep Red 660
Board A, mid3, 1.341W (4.895V, 0.273A)
Unknown source (First batch)
7.55 W/sr/m^2, 662nm

### "Blue"
Board A, bottom3, 0.902W (4.931V, 0.182A)
Unknown source (very first batch)
4.18 W/sr/m^2, 462nm

### "Yellow"
Board A, mid2, 2.162W (4.829V, 0.447A)
YinYao Yellow 590-592 "3535XPE"
1.55 W/sr/m^2, 602nm

### 560-565 Green
Board A, bottom2, 1.211W (4.908V, .250A)
HF3534D120-3W-560-0745J9 560-565
1.2 W/sr/m^2, 552nm

### "Blue"
Board A, bottom1, .761W (4.946V, .152A)
3W3535BH470B (or is it 3W3535GH470B?) Blue Axinide 470-475
4.4 W/sr/m^2, 468nm

### "Golden"
Board A, mid1, LOW POWER 1.579W (4.879V, .372A)
3535XPE Yin Yao 1900k 3.0-3.2V Golden
1.56W/sr/m^2, SPREAD 622nm

## Board B

### 590-595 Yellow
Board B, top1, 1.38W (4.893V, 0.282A)
3W3535GH590Y
1.19W/sr/m^2 596nm

### "Red" 
Board B, top2, 1.72W (4.865V, 0.353A)
Unknown Red
8.72W/sr/m^2 634nm

### "Yellow"
Board B, top3, 1.40W (4.88V, 0.286A)
Yellow
1.47W/sr/m^2 598nm

### 590-595 Yellow 
Board B, top4, 1.37W (4.89V, .283A)
3W3535GH590Y Yellow 590-595nm Aixinde
1.33W/sr/m^2 596nm

### UV395
Board B, mid3, .722W (4.95V, .144A)
First Batch
3.46W/sr/m^2 398nm

### "440-445"
Board b, bottom3, .810W (4.94V, .161A)
First Batch
2.52 438

### "Golden"
Board B, mid2, LOW POWER 1.55W (4.875V, .312A)
First Batch
1.73 SPREAD 604

## "500-510"
Board b bottom2, 1.241W (4.91V, .258A)
First batch
3.2 498nm

## "460-470"
Board b, bottom1, .688W (4.95V, .140A)
First Batch
3.4 462nm

## 440-445 "Royal Blue"
Board b, mid1, .588W (4.95V, .136A)
First BAtch
3.86 438nm


