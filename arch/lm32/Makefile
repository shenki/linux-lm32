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

head-y := arch/lm32/kernel/head.o

core-y	+= arch/lm32/
core-y	+= arch/lm32/platforms/
libs-y	+= arch/lm32/lib/

boot := arch/lm32/boot

# Are we making a simpleImage.<boardname> target? If so, crack out the boardname
DTB:=$(subst simpleImage.,,$(filter simpleImage.%, $(MAKECMDGOALS)))

ifneq ($(DTB),)
	core-y  += $(boot)/
endif

export DTB

vmlinux.bin: vmlinux
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

simpleImage.%: vmlinux
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

%.dtb:
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

archclean:
	@$(MAKE) $(clean)=$(boot)

define archhelp
	echo '  vmlinux.bin          - Raw binary boot image'
	echo '  simpleImage.<dt>     - Raw binary boot image'
	echo
	echo '  These will be default as apropriate for a configured platform.'
endef

MRPROPER_FILES += $(boot)/simpleImage.*
