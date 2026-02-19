# User program Makefile
CROSS_COMPILE = aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS = -Wall -O2 -ffreestanding -nostdlib -nostartfiles -mcpu=cortex-a53 \
         -mgeneral-regs-only -I. -fPIC

# User library sources
LIB_C   = lib/printf.c
LIB_S   = lib/crt0.S lib/syscall.S

LIB_OBJS = $(LIB_C:.c=.o) $(LIB_S:.S=.o)

# User programs (add more here)
PROGRAMS = forktest syscall_test hello

# Output directory
OUT_DIR = ../rootfs

.PHONY: all clean

all: $(PROGRAMS:%=$(OUT_DIR)/%.img)

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly files
%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# Link and create flat binary for each program
$(OUT_DIR)/%.img: %.o $(LIB_OBJS) user.ld
	$(LD) -T user.ld -o $*.elf $(filter %.o,$^)
	$(OBJCOPY) -O binary $*.elf $@
	@echo "Built user program: $@"

clean:
	rm -f lib/*.o *.o *.elf
	rm -f $(PROGRAMS:%=$(OUT_DIR)/%.img)
