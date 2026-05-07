#include "timer.h"
#include "loogal/platform.h"
#include <stdint.h>

double loogal_now_ms(void) {
uint64_t ns = loogal_platform_now_ns();
return (double)ns / 1000000.0;
}
