#ifndef MERS_GUARD
#define MERS_GUARD

#include "stdint.h"

void mers_initialize(uint32_t seed);
uint32_t mers_get_number();

#endif
