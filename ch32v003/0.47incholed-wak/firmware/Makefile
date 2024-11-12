all : flash

TARGET:=tinyoledfirmware
CH32V003FUN:=../../../rv003usb/ch32v003fun/ch32v003fun
MINICHLINK?=$(CH32V003FUN)/../minichlink
RV003USB:=../../../rv003usb/rv003usb

ADDITIONAL_C_FILES+=$(RV003USB)/rv003usb.S $(RV003USB)/rv003usb.c
EXTRA_CFLAGS:=-I$(RV003USB)/../lib -I$(RV003USB)/

option_flash :
	$(MINICHLINK)/minichlink -w option.bin option

include $(CH32V003FUN)/ch32v003fun.mk

flash : cv_flash
clean : cv_clean


