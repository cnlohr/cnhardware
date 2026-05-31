all : flash

TARGET:=ch570funprog
TARGET_MCU:=CH570
include ../../../ch32v003fun/ch32fun/ch32fun.mk

install_udev_rules : 
	sudo cp ./99-v203demo.rules /etc/udev/rules.d/
	sudo udevadm control --reload
	sudo udevadm trigger

# Below this is largely designed around the CH5xx

go_into_bootloader :
	(cd $(CH32FUN)/../examples_usb/USBFS/usbfs_device/usbhidtool && make go_into_bootloader && cd .. )

userreflash : go_into_bootloader flash

usermonitor :
	(cd $(CH32FUN)/../examples_usb/USBFS/usbfs_device/usbhidtool && make usermonitor && cd .. )

flash : cv_flash
clean : cv_clean

