all : flash

TARGET:=tuneisler
TARGET_MCU:=CH570

include ../../../ch32v003fun/ch32fun/ch32fun.mk

tofloat : tofloat.c
	gcc -o $@ $^

flash : cv_flash
clean : cv_clean
