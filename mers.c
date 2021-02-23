// Compile with gcc -std=c99 mersenne_twister.c -o /dev/random
// ps the -o /dev/random part is a joke, it's not actually meant to be a random device.
// Absolutely no warranty, Kopyleft I guess who gives a shit
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "mers.h"
uint32_t x[624];
uint32_t foo = 0;
void twist() {
    for (int k = 0; k < 624; k++) {
        x[k] = x[(k+397)%624] 
            ^ (x[k]&0x80000000 | x[(k+1)%624]&0x7fffffff) >> 1 
            ^ (x[(k+1)%624]&1) * 0x9908B0DF;
    }
}

void mers_initialize(uint32_t seed) {
    // seed of 5489 is good
    foo = 0;
    x[0] = seed;
    for (int i = 1; i < 624; i++) {
        x[i] = 1812433253UL * (x[i-1] ^ x[i-1] >> 30) + i;
    }
    twist();
}

uint32_t mers_get_number() {
    if (foo >= 624) {
        twist();
        foo = 0;
    }
    uint32_t y = x[foo++];
    y ^= y >> 11;
    y ^= y << 7 & 0x9D2C5680;
    y ^= y << 15 & 0xEFC60000;
    y ^= y >> 18;
    return y;
}
