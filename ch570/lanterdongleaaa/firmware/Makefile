all : flash

TARGET:=lanterdongleaaa
TARGET_MCU:=CH570

include ../../../ch32v003fun/ch32fun/ch32fun.mk

tofloat : tofloat.c
	gcc -o $@ $^

cycleflash : lanterdongleaaa.bin
	$(MINICHLINK)/minichlink -3
	$(MINICHLINK)/minichlink -35w lanterdongleaaa.bin flash -b

watch : tofloat
	$(MINICHLINK)/minichlink -T | pv | ./tofloat | dyngraph > /dev/null

#sudo setbaud /dev/ttyACM0 1500000 
watchserial : tofloat
	sudo stty -F /dev/ttyACM0 921600 -echo raw && cat /dev/ttyACM0  | pv | ./tofloat B | dyngraph > /dev/null

fromsaleae : tofloat
	cat todump.txt | cut -f2 -d, > todump.hex
	xxd -r -p todump.hex | ./tofloat B

flash : cv_flash
clean : cv_clean
