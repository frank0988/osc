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
ROOTFS_DIR = rootfs

# --- 編譯參數 ---
# -MMD -MP: 自動生成依賴檔 (.d)，當 header 修改時會自動重新編譯
CFLAGS = -Wall -O2 -ffreestanding -nostdlib -nostartfiles -mcpu=cortex-a53 \
         -I$(INC_DIR) -g -mgeneral-regs-only -MMD -MP

# --- 檔案搜尋 (使用 find 遞迴搜尋) ---

# 1. Drivers (共用)
DRV_SRCS_C = $(shell find $(SRC_DIR)/drivers -type f -name '*.c')
DRV_SRCS_S = $(shell find $(SRC_DIR)/drivers -type f -name '*.S' -o -name '*.s')
DRV_SRCS_s_UPPER = $(filter %.S, $(DRV_SRCS_S))
DRV_SRCS_s_LOWER = $(filter %.s, $(DRV_SRCS_S))
DRV_OBJS   = $(DRV_SRCS_C:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) \
             $(DRV_SRCS_s_UPPER:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_S.o) \
             $(DRV_SRCS_s_LOWER:$(SRC_DIR)/%.s=$(BUILD_DIR)/%_s.o)

# 2. Bootloader
BOOT_SRCS_C = $(shell find $(SRC_DIR)/bootloader -type f -name '*.c')
BOOT_SRCS_S = $(shell find $(SRC_DIR)/bootloader -type f -name '*.S' -o -name '*.s')
BOOT_SRCS_s_UPPER = $(filter %.S, $(BOOT_SRCS_S))
BOOT_SRCS_s_LOWER = $(filter %.s, $(BOOT_SRCS_S))
BOOT_OBJS   = $(BOOT_SRCS_C:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) \
              $(BOOT_SRCS_s_UPPER:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_S.o) \
              $(BOOT_SRCS_s_LOWER:$(SRC_DIR)/%.s=$(BUILD_DIR)/%_s.o) \
              $(DRV_OBJS)

# 3. Kernel (包含 sched, trap, mm 等子目錄)
KERN_SRCS_C = $(shell find $(SRC_DIR)/kernel -type f -name '*.c')
KERN_SRCS_S = $(shell find $(SRC_DIR)/kernel -type f -name '*.S' -o -name '*.s')
KERN_SRCS_s_UPPER = $(filter %.S, $(KERN_SRCS_S))
KERN_SRCS_s_LOWER = $(filter %.s, $(KERN_SRCS_S))
KERN_OBJS   = $(KERN_SRCS_C:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) \
              $(KERN_SRCS_s_UPPER:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_S.o) \
              $(KERN_SRCS_s_LOWER:$(SRC_DIR)/%.s=$(BUILD_DIR)/%_s.o) \
              $(DRV_OBJS)

# --- Linker Scripts & Images ---
BOOT_LD   = $(CONFIG_DIR)/bootloader.ld
KERN_LD   = $(CONFIG_DIR)/kernel.ld

BOOT_ELF  = $(BUILD_DIR)/bootloader.elf
BOOT_IMG  = bootloader.img

KERN_ELF  = $(BUILD_DIR)/kernel8.elf
KERN_IMG  = kernel8.img
CPIO_IMG  = $(CONFIG_DIR)/initramfs.cpio

# --- 主要目標 ---
.PHONY: all clean run_kernel run_bootloader gdb cpio directories

all: directories $(BOOT_IMG) $(KERN_IMG)

# 建立 build 目錄結構 (雖然下面的 mkdir -p 也會做，但這樣比較保險)
directories:
	@mkdir -p $(BUILD_DIR)

# --- 通用編譯規則 (Pattern Rules) ---
# 這些規則會自動匹配 src/ 下任何子目錄的檔案，並在 build/ 下建立對應目錄

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%_S.o: $(SRC_DIR)/%.S
	@mkdir -p $(dir $@)
	@echo "Assembling: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.s
	@mkdir -p $(dir $@)
	@echo "Assembling: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# --- 連結規則 ---

$(BOOT_IMG): $(BOOT_OBJS) $(BOOT_LD)
	@echo "Linking Bootloader..."
	@$(LD) -T $(BOOT_LD) -o $(BOOT_ELF) $(BOOT_OBJS) 
	@$(OBJCOPY) -O binary $(BOOT_ELF) $(BOOT_IMG)

$(KERN_IMG): $(KERN_OBJS) $(KERN_LD)
	@echo "Linking Kernel..."
	@$(LD) -T $(KERN_LD) -o $(KERN_ELF) $(KERN_OBJS) -Map $(BUILD_DIR)/kernel.map
	@$(OBJCOPY) -O binary $(KERN_ELF) $(KERN_IMG)

# --- CPIO 打包 ---
cpio:
	@echo "Packing initramfs..."
	@cd $(ROOTFS_DIR) && find . | cpio -o -H newc > ../$(CPIO_IMG)

# --- 清理 ---
clean:
	rm -rf $(BUILD_DIR) $(BOOT_IMG) $(KERN_IMG) $(CPIO_IMG)

# --- 執行與除錯 ---

# 執行 Kernel
run: $(KERN_IMG) $(CPIO_IMG)
	$(QEMU) -M raspi3b -kernel $(KERN_IMG) \
		-initrd $(CPIO_IMG) \
		-dtb $(CONFIG_DIR)/bcm2710-rpi-3-b-plus.dtb \
		-serial null -serial stdio \
		-display none

# 執行 Bootloader
run_boot: $(BOOT_IMG) $(CPIO_IMG)
	$(QEMU) -M raspi3b -kernel $(BOOT_IMG) \
		-initrd $(CPIO_IMG) \
		-dtb $(CONFIG_DIR)/bcm2710-rpi-3-b-plus.dtb \
		-serial null -serial pty \
		-display none

# GDB Server
run_debug: $(KERN_IMG) $(CPIO_IMG)
	$(QEMU) -M raspi3b \
		-kernel $(KERN_IMG) \
		-initrd $(CPIO_IMG) \
		-dtb $(CONFIG_DIR)/bcm2710-rpi-3-b-plus.dtb \
		-serial null -serial stdio \
		-s -S \
		-display none

# GDB Client (自動載入符號表)
gdb:
	gdb-multiarch -x gdb_script

# 引入 dependency files (.d)
-include $(shell find $(BUILD_DIR) -name '*.d' 2>/dev/null)