all : flash

TARGET:=firmware
TARGET_MCU:=CH572
TARGET_MCU_PACKAGE:=CH572Q


# in case we ever want to register volatile unsigned switch_time      asm ("s11");
#CFLAGS+=-Wno-volatile-register-var -ffixed-s11 -ffixed-s10 -ffixed-s9 -ffixed-s8 -g -Os -flto -ffunction-sections -fdata-sections -fmessage-length=0 -msmall-data-limit=8


include ../../../ch32v003fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean
