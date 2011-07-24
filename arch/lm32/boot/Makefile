#
# arch/lm32/boot/Makefile
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#

targets        := vmlinux.bin

ifneq ($(DTB),)
obj-y += $(DTB).dtb.o
endif

quiet_cmd_cp = CP      $< $@$2
      cmd_cp = cat $< >$@$2 || (rm -f $@ && echo false)

$(obj)/simpleImage.%: vmlinux FORCE
	$(call if_changed,objcopy)

$(obj)/vmlinux.bin: vmlinux FORCE
	$(call if_changed,objcopy)
	@echo 'Kernel: $@ is ready' ' (#'`cat .version`')'

# Rule to build device tree blobs
$(obj)/%.dtb: $(src)/dts/%.dts
	$(call cmd,dtc)

clean-files += vmlinux.bin *.dtb.S
