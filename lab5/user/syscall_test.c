#include "lib/syscall.h"

// ============================================================
// Helper: 簡易字串長度
// ============================================================
static int strlen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

// ============================================================
// Test 1: SYS_GETPID
// ============================================================
static void test_getpid(void) {
    printf("========== Test: getpid ==========\n");
    int pid = get_pid();
    printf("  My PID = %d\n", pid);
    if (pid > 0)
        printf("  [PASS] PID is positive\n");
    else
        printf("  [FAIL] PID should be > 0, got %d\n", pid);
}

// ============================================================
// Test 2: SYS_UART_WRITE
// ============================================================
static void test_uart_write(void) {
    printf("========== Test: uart_write ==========\n");
    const char *msg = "  uart_write direct output\n";
    int len = strlen(msg);
    long ret = uart_write(msg, len);
    if (ret == len)
        printf("  [PASS] uart_write returned %d (expected %d)\n", (int)ret, len);
    else
        printf("  [FAIL] uart_write returned %d (expected %d)\n", (int)ret, len);
}

// ============================================================
// Test 3: SYS_UART_READ
// ============================================================
static void test_uart_read(void) {
    printf("========== Test: uart_read ==========\n");
    printf("  Please type one character: ");
    char buf[2];
    buf[0] = 0;
    buf[1] = 0;
    uart_read(buf, 1);
    printf("\n  You typed: '%s' (0x%x)\n", buf, (unsigned long)buf[0]);
    if (buf[0] != 0)
        printf("  [PASS] Read a character successfully\n");
    else
        printf("  [FAIL] Did not read any character\n");
}

// ============================================================
// Test 4: SYS_FORK + SYS_EXIT + SYS_GETPID
// (已有 forktest，這裡做簡單的驗證)
// ============================================================
static void test_fork(void) {
    printf("========== Test: fork + exit ==========\n");
    int parent_pid = get_pid();
    int ret = fork();
    if (ret == 0) {
        // child
        int child_pid = get_pid();
        printf("  [CHILD] pid=%d, parent was %d\n", child_pid, parent_pid);
        if (child_pid != parent_pid)
            printf("  [PASS] Child has different PID\n");
        else
            printf("  [FAIL] Child PID should differ from parent\n");
        exit();
    } else if (ret > 0) {
        // parent
        printf("  [PARENT] pid=%d, child pid=%d\n", parent_pid, ret);
        if (ret != parent_pid)
            printf("  [PASS] fork returned child PID\n");
        else
            printf("  [FAIL] fork returned same PID as parent\n");
        // 等 child 執行完
        delay(3000000);
    } else {
        printf("  [FAIL] fork returned negative: %d\n", ret);
    }
}

// ============================================================
// Test 5: SYS_EXEC
// ============================================================
static void test_exec(void) {
    printf("========== Test: exec ==========\n");
    int ret = fork();
    if (ret == 0) {
        // child: exec into hello.img
        printf("  [CHILD] About to exec hello.img...\n");
        int r = exec("hello.img");
        // 如果 exec 成功，不會到這裡
        printf("  [FAIL] exec returned %d (should not return on success)\n", r);
        exit();
    } else if (ret > 0) {
        printf("  [PARENT] Waiting for child (pid %d) to exec...\n", ret);
        delay(5000000);
        printf("  [PARENT] Done waiting. If you saw 'Hello from exec!' above, exec works!\n");
    } else {
        printf("  [FAIL] fork for exec test failed: %d\n", ret);
    }
}

// ============================================================
// Main
// ============================================================
int main() {
    printf("\n====================================\n");
    printf("  System Call Test Suite\n");
    printf("====================================\n\n");

    test_getpid();
    test_uart_write();
    test_fork();
    test_exec();

    // uart_read 放最後，因為需要使用者互動
    test_uart_read();

    printf("\n====================================\n");
    printf("  All tests completed!\n");
    printf("====================================\n");
    return 0;
}
