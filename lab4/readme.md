意識到atomic的重要性，例如在修改timer linked list 但被interupt
還有FIFO 的 Race Condition (競態條件)


FIFO 的 Race Condition (競態條件)

現象：UART 輸出的文字亂碼，或 Buffer 指標錯亂。

原因：你的 handle_mini_uart_irq 會修改 FIFO 的 head/tail 索引。如果主程式（Main Loop）正在 printf 到一半被中斷，主程式的索引計算會被 ISR 覆蓋。

修正：主程式在呼叫 uart_send 或操作 FIFO 時，必須先 關閉中斷 (Disable IRQ)，操作完再開啟 但這個不太確定 因爲spsc的情況下 

還有sync handler 沒有處理得eret