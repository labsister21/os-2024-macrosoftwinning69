#include "header/math/math.h"

uint32_t ceil32(float n) {
    if ((int)n == n) return (int)n;
    return (int)n + 1;
}