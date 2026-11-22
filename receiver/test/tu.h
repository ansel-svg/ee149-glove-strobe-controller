#ifndef TU_H
#define TU_H
#include <stdio.h>
static int tu_fails = 0;
static int tu_passes = 0;
#define CHECK(c, msg) do { if (c) { tu_passes++; printf("PASS %s\n", msg); } else { tu_fails++; printf("FAIL %s\n", msg); } } while (0)
#define TU_RESULT() do { printf("RESULT: %d passed, %d failed\n", tu_passes, tu_fails); return tu_fails != 0; } while (0)
#endif
