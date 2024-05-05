#include "header/math/math.h"

uint32_t ceil32(float n) {
    if ((int)n == n) return (int)n;
    return (int)n + 1;
}

uint32_t ceil_div(uint32_t a, uint32_t b) {
    return ceil32(a / (float) b);
}