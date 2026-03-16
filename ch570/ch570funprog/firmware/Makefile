all : flash

TARGET:=ch570funprog
TARGET_MCU:=CH570
TARGET_MCU_PACKAGE:=CH570Q

include ../../../ch32v003fun/ch32fun/ch32fun.mk

bootloader :
#	./$(TARGET).py -b

flash : bootloader cv_flash
clean : cv_clean
