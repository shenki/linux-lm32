#
# arch/lm32/kernel/Makefile
#

extra-y := head.o vmlinux.lds
obj-y := \
	setup.o traps.o signal.o time.o \
	ptrace.o irq.o process.o entry.o \
	flat.o cpuinfo.o prom.o platform.o

obj-$(CONFIG_MODULES)       += module.o

