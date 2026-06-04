all : flash

TARGET:=ch570funprog
TARGET_MCU:=CH570
include ../../../ch32v003fun/ch32fun/ch32fun.mk

install_udev_rules : 
	sudo cp ./99-ch570funprog.rules /etc/udev/rules.d/
	sudo udevadm control --reload
	sudo udevadm trigger

# Below this is largely designed around the CH5xx

go_into_bootloader :
	(cd $(CH32FUN)/../examples_usb/USBFS/usbfs_device/usbhidtool && make VID=1209 PID=570f go_into_bootloader && cd .. )

userflash : go_into_bootloader flash

usermonitor :
	(cd $(CH32FUN)/../examples_usb/USBFS/usbfs_device/usbhidtool && make VID=1209 PID=570f  usermonitor && cd .. )

flash : cv_flash
clean : cv_clean

