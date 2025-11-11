# Makefile for CPU Scheduler Monitor Kernel Module

obj-m += sched_monitor.o

# Kernel source directory (auto-detected)
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Build flags
ccflags-y := -Wall -Wextra

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f test_cpu test_io test_mixed *.o

# Install module (requires root)
install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

# Load module
load:
	sudo insmod sched_monitor.ko

# Unload module
unload:
	sudo rmmod sched_monitor

# View module info
info:
	modinfo sched_monitor.ko

# View statistics
stats:
	cat /proc/sched_stats

# View kernel log messages
log:
	dmesg | tail -20

# Build test programs
tests: test_cpu test_io test_mixed

test_cpu: test_cpu.c
	gcc -O2 -o test_cpu test_cpu.c -lpthread

test_io: test_io.c
	gcc -O2 -o test_io test_io.c

test_mixed: test_mixed.c
	gcc -O2 -o test_mixed test_mixed.c -lpthread

.PHONY: all clean install load unload info stats log tests
