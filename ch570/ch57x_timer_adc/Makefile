all : flash

TARGET:=ch57x_timer_adc
TARGET_MCU:=CH570

include ../../ch32v003fun/ch32fun/ch32fun.mk

tofloat : tofloat.c
	gcc -o $@ $^

watch : tofloat
	$(MINICHLINK)/minichlink -T | pv | ./tofloat | dyngraph > /dev/null


flash : cv_flash
clean : cv_clean
