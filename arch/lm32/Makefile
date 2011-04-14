#
# arch/lm32/Makefile
#
# (C) Copyright 2007, Theobroma Systems <www.theobroma-systems.com>
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

board-$(CONFIG_BOARD_MILKYMIST_ONE)	:= milkymist-one
board-$(CONFIG_BOARD_MILKYMIST_SOC)	:= milkymist-soc
BOARD := $(board-y)

export BOARD

cflags-y := -mmultiply-enabled -mdivide-enabled \
	-mbarrel-shift-enabled -msign-extend-enabled

KBUILD_AFLAGS += $(cflags-y)
KBUILD_CFLAGS += $(cflags-y)

OBJCOPYFLAGS	:=-O binary -R .note -R .comment -S

ASM_GENERATED_DIR	:= $(srctree)/arch/lm32/include/generated
LINUXINCLUDE		+= -I$(ASM_GENERATED_DIR)

ASM_GENERIC_HEADERS	:= atomic.h auxvec.h
ASM_GENERIC_HEADERS	+= bitsperlong.h bug.h bugs.h
ASM_GENERIC_HEADERS	+= cache.h checksum.h cmpxchg.h cmpxchg-local.h cputime.h current.h
ASM_GENERIC_HEADERS	+= device.h div64.h dma.h
ASM_GENERIC_HEADERS	+= emergency-restart.h errno.h
ASM_GENERIC_HEADERS	+= fb.h fcntl.h futex.h
ASM_GENERIC_HEADERS	+= hardirq.h hw_irq.h
ASM_GENERIC_HEADERS	+= ioctl.h ioctls.h io.h ipcbuf.h irq_regs.h
ASM_GENERIC_HEADERS	+= kdebug.h kmap_types.h
ASM_GENERIC_HEADERS	+= linkage.h local.h
ASM_GENERIC_HEADERS	+= mman.h mmu_context.h module.h msgbuf.h
ASM_GENERIC_HEADERS	+= param.h pci.h percpu.h pgalloc.h poll.h posix_types.h
ASM_GENERIC_HEADERS	+= resource.h
ASM_GENERIC_HEADERS	+= scatterlist.h segment.h sembuf.h setup.h shmbuf.h
ASM_GENERIC_HEADERS	+= shmparam.h siginfo.h signal.h socket.h sockios.h statfs.h
ASM_GENERIC_HEADERS	+= stat.h string.h swab.h syscalls.h
ASM_GENERIC_HEADERS	+= termbits.h termios.h tlbflush.h topology.h types.h
ASM_GENERIC_HEADERS	+= ucontext.h unaligned.h user.h

archprepare:
ifneq ($(ASM_GENERATED_DIR), $(wildcard $(ASM_GENERATED_DIR)))
	$(Q)mkdir -p $(ASM_GENERATED_DIR)/asm
	$(Q)$(foreach a, $(ASM_GENERIC_HEADERS),	\
		echo '#include <asm-generic/$a>'	\
			> $(ASM_GENERATED_DIR)/asm/$a; )
endif

MRPROPER_DIRS += $(ASM_GENERATED_DIR)

head-y := arch/lm32/kernel/head.o arch/lm32/kernel/init_task.o

core-y	+= arch/lm32/
libs-y	+= arch/lm32/lib/

vmlinux.bin: vmlinux
	$(OBJCOPY) $(OBJCOPYFLAGS) $< $@

vmlinux.gz: vmlinux.bin
	cat $< |gzip >$@
