all : flash

TARGET:=firmware

TARGET_MCU?=CH32V003
include ../../../ch32v003fun/ch32fun/ch32fun.mk

optionbytes :
	#$(MINICHLINK)/minichlink -w +a55af708 option # Default (But what does it mean??)
	#$(MINICHLINK)/minichlink -w +a55aff00 option # Enable bootloader, disable RESET
	#$(MINICHLINK)/minichlink -w +a55aef10 option # Enable bootloader, enable RESET
	$(MINICHLINK)/minichlink -w +a55adf20 option # Disable bootloader, disable RESET (Reset is GPIO)
	#$(MINICHLINK)/minichlink -w +a55acf30 option # Disable bootloader, enable RESET


flash : cv_flash
clean : cv_clean


