obj-y += mm1.dtb.o

clean-files := *.dtb.S

$(obj)/%.dtb: $(src)/dts/%.dts
	$(call cmd,dtc)

