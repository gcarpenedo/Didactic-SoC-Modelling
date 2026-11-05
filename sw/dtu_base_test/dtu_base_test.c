#include <stdint.h>
#include "soc_ctrl.h"

#define DTU_BASE 0x01052000u

int main(void) {
    PMOD_CTRL = 2;
    SS2_CTRL = 1;
    RST_CTRL = 9;

    *(volatile uint32_t *)DTU_BASE = 0xA55A1234u;
    return 0;
}