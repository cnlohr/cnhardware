
all : flash

TARGET:=controller
CH32FUN:=../../ch32v003fun/ch32fun
TARGET_MCU:=CH32V003

ADDITIONAL_C_FILES+=../../rv003usb/rv003usb/rv003usb.S ../../rv003usb/rv003usb/rv003usb.c
EXTRA_CFLAGS:=-I../../rv003usb/lib -I../../rv003usb/rv003usb

optionbytes :
	$(MINICHLINK)/minichlink -w +a55aff00 option 
	# Enable bootloader, disable RESET

	#$(MINICHLINK)/minichlink -w +a55af708 option # Default (But what does it mean??)
	#$(MINICHLINK)/minichlink -w +a55aef10 option # Enable bootloader, enable RESET
	#$(MINICHLINK)/minichlink -w +a55adf20 option # Disable bootloader, disable RESET
	#$(MINICHLINK)/minichlink -w +a55acf30 option # Disable bootloader, enable RESET


include $(CH32FUN)/ch32fun.mk

flash : cv_flash
clean : cv_clean


