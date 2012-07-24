#
# arch/lm32/kernel/Makefile
#

extra-y := head.o vmlinux.lds
obj-y := \
	sys_lm32.o setup.o traps.o signal.o time.o \
	ptrace.o irq.o process.o entry.o \
	flat.o sys_call_table.o cpuinfo.o prom.o \
	platform.o

obj-$(CONFIG_MODULES)       += module.o

