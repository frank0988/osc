# --- 工具鏈設定 ---
CROSS_COMPILE = aarch64-linux-gnu-
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
QEMU    = qemu-system-aarch64

# --- 目錄設定 ---
SRC_DIR   = src
INC_DIR   = include
BUILD_DIR = build
CONFIG_DIR = config

# --- 編譯參數 ---
# 整合自 Makefile 1 & 2 [cite: 1, 3]
CFLAGS = -Wall -O2 -ffreestanding -nostdlib -nostartfiles -mcpu=cortex-a53 -I$(INC_DIR)

# --- 檔案定義 ---
# 共用驅動程式 (Shared Drivers)
DRV_C_SRCS = $(wildcard $(SRC_DIR)/drivers/*.c)
DRV_S_SRCS = $(wildcard $(SRC_DIR)/drivers/*.S)
DRV_C_OBJS = $(patsubst $(SRC_DIR)/drivers/%.c, $(BUILD_DIR)/drivers/%.o, $(DRV_C_SRCS))
DRV_S_OBJS = $(patsubst $(SRC_DIR)/drivers/%.S, $(BUILD_DIR)/drivers/%.o, $(DRV_S_SRCS))
DRV_OBJS = $(DRV_C_OBJS) $(DRV_S_OBJS)

# Bootloader 相關
BOOT_SRCS = $(wildcard $(SRC_DIR)/bootloader/*.c) $(wildcard $(SRC_DIR)/bootloader/*.S)
BOOT_OBJS = $(patsubst $(SRC_DIR)/bootloader/%, $(BUILD_DIR)/bootloader/%.o, $(BOOT_SRCS))
BOOT_ELF  = $(BUILD_DIR)/bootloader.elf
BOOT_IMG  = bootloader.img
BOOT_LD   = $(CONFIG_DIR)/bootloader.ld # 請將 bootloader 的 linker.ld 移至此處 [cite: 3]

# Kernel 相關
KERN_SRCS = $(wildcard $(SRC_DIR)/kernel/*.c) $(wildcard $(SRC_DIR)/kernel/*.S)
KERN_OBJS = $(patsubst $(SRC_DIR)/kernel/%, $(BUILD_DIR)/kernel/%.o, $(KERN_SRCS))
KERN_ELF  = $(BUILD_DIR)/kernel8.elf
KERN_IMG  = kernel8.img
KERN_LD   = $(CONFIG_DIR)/kernel.ld     # 請將 kernel 的 linker.ld 移至此處 

# --- 主要目標 ---
.PHONY: all clean run_kernel run_bootloader

all: $(BOOT_IMG) $(KERN_IMG)

# --- 編譯規則 ---

# 編譯驅動程式
$(BUILD_DIR)/drivers/%.o: $(SRC_DIR)/drivers/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/drivers/%.o: $(SRC_DIR)/drivers/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 編譯 Bootloader 原始碼
$(BUILD_DIR)/bootloader/%.o: $(SRC_DIR)/bootloader/%
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 編譯 Kernel 原始碼
$(BUILD_DIR)/kernel/%.o: $(SRC_DIR)/kernel/%
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 連結 Bootloader (包含共用驅動)
$(BOOT_IMG): $(BOOT_OBJS) $(DRV_OBJS)
	$(LD) -T $(BOOT_LD) -o $(BOOT_ELF) $(BOOT_OBJS) $(DRV_OBJS)
	$(OBJCOPY) -O binary $(BOOT_ELF) $(BOOT_IMG)

# 連結 Kernel (包含共用驅動)
$(KERN_IMG): $(KERN_OBJS) $(DRV_OBJS)
	$(LD) -T $(KERN_LD) -o $(KERN_ELF) $(KERN_OBJS) $(DRV_OBJS)
	$(OBJCOPY) -O binary $(KERN_ELF) $(KERN_IMG)

# --- 清理 ---
clean:
	rm -rf $(BUILD_DIR) $(BOOT_IMG) $(KERN_IMG)

# --- 執行指令 (整合自原本的 run 目標) ---

# 執行 Kernel [cite: 2]
run_kernel: $(KERN_IMG)
	$(QEMU) -M raspi3b -kernel $(KERN_IMG) \
		-initrd $(CONFIG_DIR)/initramfs.cpio \
		-dtb $(CONFIG_DIR)/bcm2710-rpi-3-b-plus.dtb \
		-serial null -serial stdio \
		-display none

# 執行 Bootloader (使用原本的 pty 設定) [cite: 4]
run_bootloader: $(BOOT_IMG)
	$(QEMU) -M raspi3b -kernel $(BOOT_IMG) \
		-dtb bcm2710-rpi-3-b-plus.dtb \
		-initrd initramfs.cpio \
		-serial pty \
		-display none