# Programs built by this makefile
RUN_PROGRAM   = clock.bin
TEST_PROGRAM  = test_gpio_timer.bin

MY_MODULE_SOURCES = gpio.c timer.c timer_asm.s

PROGRAMS      = $(RUN_PROGRAM) $(TEST_PROGRAM)

all: $(PROGRAMS)

# Flags for compile and link
ARCH 	= -march=rv64im -mabi=lp64
ASFLAGS = $(ARCH)
CFLAGS 	= $(ARCH) -g -Og -I$$CS107E/include $$warn $$freestanding
LDFLAGS = -nostdlib -L$$CS107E/lib -T memmap.ld
LDLIBS 	= -lmango_gcc

# Clock-specific variable to set the duration on make command-line
# Selectively apply phony to force rebuild of clock whenever set duration
ifdef DURATION
clock.o: CFLAGS += -DDURATION=$(DURATION)
.PHONY: clock.c
endif

# Common objects for the programs built by this makefile
SOURCES = start.s cstart.c $(MY_MODULE_SOURCES)
OBJECTS = $(addsuffix .o, $(basename $(SOURCES)))

# Rules and recipes for all build steps

# Extract raw binary from elf executable
%.bin: %.elf
	riscv64-unknown-elf-objcopy $< -O binary $@

# Link program executable from program.o and all common objects
%.elf: $(OBJECTS) %.o
	riscv64-unknown-elf-ld $(LDFLAGS) $^ $(LDLIBS) -o $@

# Compile C source to object file
%.o: %.c
	riscv64-unknown-elf-gcc $(CFLAGS) -c $< -o $@

# Assemble asm source to object file
%.o: %.s
	riscv64-unknown-elf-as $(ASFLAGS) $< -o $@

# Use target `list` to print disassembly listing for entire program
list: clock.elf
	riscv64-unknown-elf-objdump $(OBJDUMP_FLAGS) -d $<

# Use target `fnname.list` to print disasembly for single named function
%.list: clock.elf
	riscv64-unknown-elf-objdump $(OBJDUMP_FLAGS) --disassemble=$* $<

# Build and run the application binary
run: $(RUN_PROGRAM)
	mango-run $<

# Build and run the test binary
test: $(TEST_PROGRAM)
	mango-run $<

# Remove all build products
clean:
	rm -f *.o *.bin *.elf

.PHONY: all clean run test list
.PRECIOUS: %.elf %.o

# disable built-in rules (they are not used)
.SUFFIXES:

export warn = -Wall -Wpointer-arith -Wwrite-strings -Werror \
              -Wno-error=unused-function -Wno-error=unused-variable \
              -fno-diagnostics-show-option
export freestanding = -ffreestanding -nostdinc \
                      -isystem $(shell riscv64-unknown-elf-gcc -print-file-name=include)
OBJDUMP_FLAGS = --no-show-raw-insn --no-addresses --disassembler-color=terminal --visualize-jumps=extended-color
