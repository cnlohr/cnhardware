all : flash

SWADGE:=../../../..

.PHONY : emu
TARGET_MCU?=CH32V003

TARGET:=sao
CH32FUN:=../../../ch32v003fun/ch32fun
MINICHLINK?=$(CH32FUN)/../minichlink

ADDITIONAL_C_FILES+=
EXTRA_CFLAGS:=-I$(RV003USB)/../lib -I$(RV003USB)/

swadge2025font.h : tools/microgenfont
	tools/microgenfont $(SWADGE)/assets/fonts/sonic.font.png > $@

tools/microgenfont : tools/microgenfont.c
	gcc -o $@ $^ -lm

emu :
	gcc sao.c -g -O3 -o $@ $^ -DEMU -ICH32FUN/extralibs -lX11

option_flash :
	$(MINICHLINK)/minichlink -w option.bin option

include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean
	rm -rf tools/microgenfont emu

