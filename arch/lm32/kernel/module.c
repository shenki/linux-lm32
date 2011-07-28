#include <linux/moduleloader.h>
#include <linux/elf.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/kernel.h>

static int relocate_at(int rtype, unsigned* insn_addr, unsigned target_addr)
{
	unsigned insn;
	unsigned opcode;

	switch (rtype) {
	case R_LM32_32:
		/* we can't really test whether the original data is an instruction
			 that could be relocated, so we simply overwrite the data that's
			 there */
		//printk("R_LM32_32: overwriting 0x%08x with 0x%08x @ 0x%08x.\n", *insn_addr, target_addr, (unsigned)insn_addr);
		*insn_addr = target_addr;
		return 0; /* never fails */

	case R_LM32_HI16:
		/* the HI16 case is permitted on mvhi (orhi) instructions only! */
		insn = *insn_addr;
		if ((insn >> 26) == 0x1e) { /* the opcode for orhi is 6b'011110 (0x1e) */
			insn &= 0xffff0000;
			insn |= (target_addr >> 16);
			*insn_addr = insn;
			return 0;
		}
		//printk("insn word 0x%08x\n", insn);
		break;

	case R_LM32_LO16:
		insn = *insn_addr;
		if ((insn >> 26) == 0x0e) { /* the opcode for ori is 6b'001110 (0x0e) */
			insn &= 0xffff0000;
			insn |= (target_addr & 0xffff);
			*insn_addr = insn;
			return 0;
		}
		break;

	case R_LM32_CALL:
		insn = *insn_addr;
		opcode = insn >> 26;
		if (opcode == 0x3e /* calli */)
		{
			/* install a calli to the given target_addr relative from the instruction
			 * address */
			long imm26 = (target_addr - (long)insn_addr);
			imm26 = (imm26 >> 2) & 0x03FFFFFF;
			insn = 0xF8000000 + imm26;
			*insn_addr = insn;
			return 0;
		}
		break;

	default:
		printk("ignoring relocation type %d @ 0x%08x\n", rtype, *insn_addr);
	}

	return -ENOEXEC;
}

static int _apply_relocate(Elf32_Shdr *sechdrs, const char *strtab,
	unsigned int symindex, unsigned int relsec, struct module *me,
	bool add)
{
	unsigned int i;
	unsigned int target_addr;
	Elf32_Rela *rel = (void *)sechdrs[relsec].sh_addr;
	Elf32_Sym *sym;
	uint32_t *location;
	int ret;

	pr_debug("Applying relocate section %u to %u\n", relsec,
				 sechdrs[relsec].sh_info);
	for (i = 0; i < sechdrs[relsec].sh_size / sizeof(*rel); i++) {
		/* This is where to make the change */
		location = (void *)sechdrs[sechdrs[relsec].sh_info].sh_addr
			+ rel[i].r_offset;
		/* This is the symbol it is referring to.	Note that all
			 undefined symbols have been resolved.	*/
		sym = (Elf32_Sym *)sechdrs[symindex].sh_addr +
			ELF32_R_SYM(rel[i].r_info);

		target_addr = sym->st_value;
		if (add)
			target_addr += rel[i].r_addend;

		ret = relocate_at(ELF32_R_TYPE(rel[i].r_info), location,
				sym->st_value);
		if (ret)
			return ret;
	}
	return 0;
}

int apply_relocate(Elf32_Shdr *sechdrs, const char *strtab,
	unsigned int symindex, unsigned int relsec, struct module *me)
{
	return _apply_relocate(sechdrs, strtab, symindex, relsec, me, false);
}

int apply_relocate_add(Elf32_Shdr *sechdrs, const char *strtab,
	unsigned int symindex, unsigned int relsec, struct module *me)
{
	return _apply_relocate(sechdrs, strtab, symindex, relsec, me, true);
}
