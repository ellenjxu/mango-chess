# Makefile to build all named modules in library form libmymango.a

LIBMANGO_MODULE_SOURCES = gpio.c timer.c timer_asm.s strings.c printf.c backtrace.c backtrace_asm.s symtab.c malloc.c \
                       keyboard.c shell.c fb.c gl.c console.c ps2_assign7.c

all: libmymango.a

# Flags for compile and link
ARCH 	= -march=rv64im -mabi=lp64
ASFLAGS = $(ARCH)
CFLAGS 	= $(ARCH) -g -Og -I$$CS107E/include $$warn $$freestanding -fno-omit-frame-pointer

SOURCES = cstart.c $(LIBMANGO_MODULE_SOURCES)
OBJECTS = $(addsuffix .o, $(basename $(SOURCES)))

libmymango.a: $(OBJECTS)
	rm -f $@
	riscv64-unknown-elf-ar cDr $@ $(filter %.o,$^)

# Compile C source to object file
%.o: %.c
	riscv64-unknown-elf-gcc $(CFLAGS) -c $< -o $@

# Assemble asm source to object file
%.o: %.s
	riscv64-unknown-elf-as $(ASFLAGS) $< -o $@

# Remove all build products
clean:
	rm -f *.a *.o *.bin *.elf *.list

# this rule will provide better error message when
# a source file cannot be found (missing, misnamed)
$(SOURCES):
	$(error cannot find source file `$@` needed for build)

.PHONY: all clean
.PRECIOUS: %.elf %.o

# disable built-in rules (they are not used)
.SUFFIXES:

export warn = -Wall -Wpointer-arith -Wwrite-strings -Werror \
        	-Wno-error=unused-function -Wno-error=unused-variable \
        	-fno-diagnostics-show-option
export freestanding = -ffreestanding -nostdinc \
		-isystem $(shell riscv64-unknown-elf-gcc -print-file-name=include)
