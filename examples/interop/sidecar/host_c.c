/* host_c.c — C host calling the proven Lume payment_core sidecar (docs/22 G2, #196). */
#include <stdio.h>
#include <stdint.h>
#include "payment_core.h"

int main(void) {
    int64_t settled = settle_transfer(1000, 250, 15);
    int64_t total = batch_checksum(settled, 265);
    printf("c_sidecar_ok settled=%lld total=%lld\n", (long long)settled, (long long)total);
    return (settled == 735 && total == 1000) ? 0 : 1;
}
