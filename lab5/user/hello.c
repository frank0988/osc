#include "lib/syscall.h"

int main() {
    printf("Hello from exec! pid: %d\n", get_pid());
    return 0;
}
