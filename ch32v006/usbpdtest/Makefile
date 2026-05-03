all : flash

TARGET:=usbpdtest

TARGET_MCU?=CH32V006
include ../../ch32v003fun/ch32fun/ch32fun.mk

optionbytes :
	$(MINICHLINK)/minichlink -w +a55aff00 option # Enable bootloader, disable RESET


flash : cv_flash
clean : cv_clean


