/* ld script to make LM32 Linux kernel
 * Written by Takeshi Matsuya <macchan@sfc.wide.ad.jp>
 * taken from the ARM version by Martin Mares <mj@atrey.karlin.mff.cuni.cz>
 * taken from the i386 version by Russell King
 */

#include <asm-generic/vmlinux.lds.h>
#include <asm/thread_info.h>
#include <asm/page.h>

OUTPUT_FORMAT("elf32-lm32")
OUTPUT_ARCH("lm32")
ENTRY(_start)

jiffies = jiffies_64 + 4;
SECTIONS
{
	. = CONFIG_KERNEL_BASE_ADDR;
	__init_begin = .;
	HEAD_TEXT_SECTION
	INIT_TEXT_SECTION(PAGE_SIZE)
	INIT_DATA_SECTION(16)
	PERCPU_SECTION(32)
	__init_end = .;

	.text : AT(ADDR(.text) - LOAD_OFFSET) {
		_text = .;
		_stext = .;
		. = ALIGN(256);
		__exception_text_start = .;
		*(.exception.text)
		__exception_text_end = .;
		TEXT_TEXT
		SCHED_TEXT
		LOCK_TEXT
		KPROBES_TEXT
		IRQENTRY_TEXT
		_etext = .;
	}

	_sdata = .;
	RO_DATA_SECTION(PAGE_SIZE)
	RW_DATA_SECTION(32, PAGE_SIZE, THREAD_SIZE)
	_edata = .;

	EXCEPTION_TABLE(256)
	NOTES

	BSS_SECTION(4, 4, 32)
	_end = .;

	STABS_DEBUG
	DWARF_DEBUG

	DISCARDS
}
