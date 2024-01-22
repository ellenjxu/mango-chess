#
# This makefile doesn't build a program to run.
# Its only target is the `codegen.list` file
# that allows you to examine the disassembly from the compiler.
#

CFLAGS = -march=rv64im -mabi=lp64 -Og -Wall -ffreestanding
OBJDUMP_FLAGS = --no-show-raw-insn --no-addresses --disassembler-color=terminal --visualize-jumps

all : codegen.list

%.o: %.c
	riscv64-unknown-elf-gcc $(CFLAGS) -c $< -o $@

%.list: %.o
	riscv64-unknown-elf-objdump $(OBJDUMP_FLAGS) -d $<

clean:
	rm -f *.o *.bin *.elf *.list

.PHONY: all clean
