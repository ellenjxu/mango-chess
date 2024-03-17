PROGRAM = main.bin
SOURCES = re.c ringbuffer_ptr.c chess.c bt_ext.c jnxu.c chess_gui.c

all: $(PROGRAM)

# Flags for compile and link
ARCH 	= -march=rv64im -mabi=lp64
ASFLAGS = $(ARCH)
CFLAGS 	= $(ARCH) -g -Og -I$$CS107E/include $$warn $$freestanding -fno-omit-frame-pointer
LDFLAGS = -nostdlib -L$$CS107E/lib -T memmap.ld
LDLIBS 	= -lmango -lmango_gcc

OBJECTS = $(addsuffix .o, $(basename $(SOURCES)))

# Rules and recipes for all build steps

# Extract raw binary from elf executable
%.bin: %.elf
	riscv64-unknown-elf-objcopy $< -O binary $@

# Link program executable from all common objects
%.elf: %.o $(OBJECTS) libmymango.a
	riscv64-unknown-elf-gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

# Compile C source to object file
%.o: %.c
	riscv64-unknown-elf-gcc $(CFLAGS) -c $< -o $@

# Assemble asm source to object file
%.o: %.s
	riscv64-unknown-elf-as $(ASFLAGS) $< -o $@

# Build and run the application binary
run: $(PROGRAM)
	mango-run $<

brain: brain.bin
	mango-run $<

hand: hand.bin
	mango-run $<
# Remove all build products
clean:
	rm -f *.o *.bin *.elf *.list *~

lib:
	$(MAKE) -C $$CS107E/../mylib clean
	$(MAKE) -C $$CS107E/../mylib
	cp $$CS107E/../mylib/libmymango.a .

# this rule will provide better error message when
# a source file cannot be found (missing, misnamed)
$(SOURCES):
	$(error cannot find source file `$@` needed for build)

libmymango.a:
	$(error run `make lib` to build libmymango.a needed for build)

.PHONY: all clean run lib
.PRECIOUS: %.elf %.o

# disable built-in rules (they are not used)
.SUFFIXES:

export warn = -Wall -Wpointer-arith -Wwrite-strings -Werror \
              -Wno-error=unused-function -Wno-error=unused-variable \
              -fno-diagnostics-show-option
export freestanding = -ffreestanding -nostdinc \
                      -isystem $(shell riscv64-unknown-elf-gcc -print-file-name=include)
