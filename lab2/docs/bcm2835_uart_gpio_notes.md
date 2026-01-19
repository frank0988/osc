# [Bare Metal] BCM2835 GPIO 與 Mini UART 初始化筆記

**建立日期**: 2025-12-13
**硬體平台**: Raspberry Pi (BCM2835 Architecture)
**相關檔案**: `gpio.h`, `uart.c`

---

## 1. 核心觀念：GPIO Function Select (GPFSEL)

在 BCM2835 中，一個實體 Pin 腳可以有多種功能 (Input, Output, I2C, UART 等)。我們透過 **Function Select Registers (`GPFSELn`)** 來切換這些模式。

### 1.1 暫存器結構
* 每個 GPIO Pin 使用 **3 個 bits** 來定義其功能。
* 一個 32-bit 的暫存器 (`GPFSELn`) 可以管理 10 個 GPIO pins ($3 \times 10 = 30$ bits，剩下 2 bits 保留)。

| 暫存器 | 管理範圍 |
| :--- | :--- |
| `GPFSEL0` | GPIO 0 ~ 9 |
| **`GPFSEL1`** | **GPIO 10 ~ 19** (Mini UART 位於此) |
| `GPFSEL2` | GPIO 20 ~ 29 |

### 1.2 計算位移量 (Bit Shift Calculation)
目標是設定 **GPIO 14 (TX)** 和 **GPIO 15 (RX)** 為 **ALT5** 模式。

1.  **確認暫存器**：GPIO 14, 15 屬於 `GPFSEL1`。
2.  **計算索引 (Index)**：
    * GPIO 14 是該組的第 4 個 ($14 - 10 = 4$)。
    * GPIO 15 是該組的第 5 個 ($15 - 10 = 5$)。
3.  **計算位移 (Shift)**：每個 Pin 佔 3 bits。
    * GPIO 14 shift: $4 \times 3 = \mathbf{12}$
    * GPIO 15 shift: $5 \times 3 = \mathbf{15}$

### 1.3 功能對照表 (Function Table)
| 數值 (Binary) | 數值 (Decimal) | 模式 | 備註 |
| :--- | :--- | :--- | :--- |
| `000` | 0 | Input | 預設值 |
| `001` | 1 | Output | LED 等 |
| `100` | 4 | ALT0 | PL011 UART |
| **`010`** | **2** | **ALT5** | **Mini UART** |

---

## 2. C 語言位元操作慣用手型 (Bitwise Idioms)

為了安全地修改 Register 而不影響其他 Pins，必須使用 **Read-Modify-Write (RMW)** 策略。

### 步驟一：清除舊設定 (Clear)
使用 `&= ~MASK` 手法。
* **Magic Number `7`**: 二進位 `0b111`，剛好遮罩住 3 個 bits。
* **邏輯**: `~(7 << 12)` 會產生一個「除了 bit 12-14 是 0 以外，其他全為 1」的 Mask。

```c
reg = *GPFSEL1;               // Read
reg &= ~((7<<12) | (7<<15));  // Modify (Clear GPIO 14 & 15)
步驟二：寫入新設定 (Set)
使用 |= VALUE 手法。

Magic Number 2: 對應 ALT5 模式。

C

reg |= ((2<<12) | (2<<15));   // Modify (Set to ALT5)
*GPFSEL1 = reg;               // Write
3. 內部電阻設定儀式 (Pull-up/down Sequence)
BCM2835 的 Pull-up/down 電阻設定需要特定的時序 (Sequence) 來鎖存 (Latch) 設定。 目標: 關閉 (Disable) UART 腳位的內建電阻，避免訊號干擾。

操作流程 (The Ritual)
意圖宣告: 寫入 GPPUD (0=Off, 1=Down, 2=Up)。

Setup Time: 等待 150 cycles，讓控制訊號穩定。

觸發 Clock: 寫入 GPPUDCLK0，指定要套用設定的 Pin (bit 14 & 15)。

Hold Time: 等待 150 cycles，確保設定被硬體鎖入。

釋放: 清除 GPPUDCLK0，完成操作。

注意: 若省略 Clock 步驟，GPPUD 的設定將不會生效。
