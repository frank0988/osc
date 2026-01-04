CROSS_COMPILE = aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
QEMU = qemu-system-aarch64

# --- [新增] 關鍵編譯參數 ---
# -Wall: 顯示所有警告
# -O2: 進行優化
# -ffreestanding: 裸機環境環境 (沒有標準 C 函式庫)
# -nostdlib: 不連結標準庫
# -mcpu=cortex-a53: 指定 Pi 3B 的 CPU 架構
CFLAGS = -Wall -O2 -ffreestanding -nostdlib -nostartfiles -mcpu=cortex-a53

OBJS = a.o shell.o uart.o mailbox.o
ELF = kernel8.elf
IMG = kernel8.img
LINKER_SCRIPT = linker.ld

all: $(IMG)

# --- 修改重點：加入 CFLAGS ---
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJS) $(LINKER_SCRIPT)
	$(LD) -T $(LINKER_SCRIPT) -o $(ELF) $(OBJS)

$(IMG): $(ELF)
	$(OBJCOPY) -O binary $(ELF) $(IMG)

clean:
	rm -f *.o $(ELF) $(IMG)

# --- 修改重點：讓 UART 輸出到終端機 ---
# -serial null -serial stdio: 把 QEMU 的 Mini UART 對應到你的 Terminal
run: $(IMG)
	$(QEMU) -M raspi3b -kernel $(IMG) -serial null -serial stdio -display none
