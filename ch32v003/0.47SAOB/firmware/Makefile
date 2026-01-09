all : flash

SWADGE:=../../../..

.PHONY : emu

TARGET:=sao
CH32V003FUN:=ch32v003fun/ch32v003fun
MINICHLINK?=$(CH32V003FUN)/../minichlink

ADDITIONAL_C_FILES+=
EXTRA_CFLAGS:=-I$(RV003USB)/../lib -I$(RV003USB)/


ch32v003fun :
	git clone https://github.com/cnlohr/ch32v003fun

swadge2025font.h : tools/microgenfont
	tools/microgenfont $(SWADGE)/assets/fonts/sonic.font.png > $@

tools/microgenfont : tools/microgenfont.c
	gcc -o $@ $^ -lm

emu :
	gcc sao.c -g -O3 -o $@ $^ -DEMU -Ich32v003fun/extralibs -lX11

option_flash :
	$(MINICHLINK)/minichlink -w option.bin option

include $(CH32V003FUN)/ch32v003fun.mk

flash : cv_flash
clean : cv_clean
	rm -rf tools/microgenfont emu

