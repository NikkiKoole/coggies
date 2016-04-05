#ifndef RANDOM_H
#define RANDOM_H

#include "types.h"

void seed_rand(u32);
u32 random_mersenne(void);

#define rand_float() (random_mersenne() / 4294967295.0)
#define rand_int2(min, max) ((s32)(min + (max - min + 1) * rand_float()))
#define rand_int(max) (rand_int2(0, max))
#define rand_bool() (random_mersenne() > 2147483647)
#define rand_arr(array) (array[rand_int(ArrayCount(array))])
#define rand_str(string) (string[rand_int(ArrayCount(string) - 2)])

#endif
