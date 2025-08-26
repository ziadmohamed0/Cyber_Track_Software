# Makefile for RPi4 UART Kernel Module

# Module name
MODULE_NAME := rpi4_uart

# Source files
obj-m += $(MODULE_NAME).o

# Kernel source directory
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# Default target
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

# Clean target
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f Module.markers modules.order

# Install target
install: all
	sudo insmod $(MODULE_NAME).ko

# Uninstall target
uninstall:
	sudo rmmod $(MODULE_NAME)

# Show module info
info:
	modinfo $(MODULE_NAME).ko

# Show kernel log
dmesg:
	dmesg | tail -20

# Create device node (if not created automatically)
create_device:
	sudo mknod /dev/rpi4_uart c $(shell cat /proc/devices | grep rpi4_uart | cut -d' ' -f1) 0
	sudo chmod 666 /dev/rpi4_uart

# Remove device node
remove_device:
	sudo rm -f /dev/rpi4_uart

.PHONY: all clean install uninstall info dmesg create_device remove_device