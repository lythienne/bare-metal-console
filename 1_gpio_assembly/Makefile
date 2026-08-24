# Makefile for assign1 larson

# larson.bin is built from larson.s using steps below
larson.bin: larson.s
	riscv64-unknown-elf-as larson.s -o larson.o
	riscv64-unknown-elf-ld -e 0 larson.o -o larson.elf
	riscv64-unknown-elf-objcopy larson.elf -O binary larson.bin

# `make run` triggers build of larson.bin, then executes program on Pi
run: larson.bin
	mango-run larson.bin

# extension.bin is built from extension.s using steps below
extension.bin: extension.s
	riscv64-unknown-elf-as extension.s -o extension.o
	riscv64-unknown-elf-ld -e 0 extension.o -o extension.elf
	riscv64-unknown-elf-objcopy extension.elf -O binary extension.bin

# `make extension` triggers build of extension.bin, then executes program on Pi
extension: extension.bin
	mango-run extension.bin


# `make clean` removes all compiled build products
clean:
	rm -f larson.o larson.elf larson.bin extension.o extension.elf extension.bin
