# 工具鏈前綴，請確保已安裝 aarch64-linux-gnu 工具鏈
CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

# 編譯選項
CFLAGS = -Wall -ffreestanding -nostdinc -nostdlib -nostartfiles -Ibootloader
# 連結選項，使用您提供的 linker.ld
LDFLAGS = -T bootloader/linker.ld

# 原始碼路徑
SRC_DIR = bootloader
BUILD_DIR = build

# 尋找所有 .c 和 .s 檔案
C_SRCS = $(wildcard $(SRC_DIR)/*.c)
S_SRCS = $(wildcard $(SRC_DIR)/*.s)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SRCS)) \
       $(patsubst $(SRC_DIR)/%.s, $(BUILD_DIR)/%.o, $(S_SRCS))

# 目標檔案名稱
BOOTLOADER_IMG = bootloader.img
BOOTLOADER_ELF = $(BUILD_DIR)/bootloader.elf

.PHONY: all clean run

all: $(BOOTLOADER_IMG)

# 建立編譯目錄並編譯 C 檔案
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 建立編譯目錄並編譯 Assembly 檔案
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 連結並生成二進位影像檔
$(BOOTLOADER_IMG): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(BOOTLOADER_ELF)
	$(OBJCOPY) -O binary $(BOOTLOADER_ELF) $(BOOTLOADER_IMG)

# 測試 Bootloader 的 QEMU 指令
# 使用 -serial pty 以便讓 python 腳本連線
run: $(BOOTLOADER_IMG)
	qemu-system-aarch64 -M raspi3b \
		-kernel $(BOOTLOADER_IMG) \
		-display none \
		-serial null -serial pty
debug: $(BOOTLOADER_IMG)
	qemu-system-aarch64 -M raspi3b \
		-kernel $(BOOTLOADER_IMG) \
		-display none \
		-serial null -serial pty \
		-S -s

# 清除編譯產物:
clean:
	rm -rf $(BUILD_DIR) $(BOOTLOADER_IMG)

test:
	qemu-system-aarch64 -M raspi3b \
        -kernel $(BOOTLOADER_IMG) \
        -display none \
        -serial null -serial stdio
runpy: 
		python3 host/sendimg.py
