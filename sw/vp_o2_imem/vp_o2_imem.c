#include <stdint.h>

static volatile uint32_t sink;

int main(void) {
    sink = *(volatile const uint32_t *)0x01000080u;
    return 0;
}
