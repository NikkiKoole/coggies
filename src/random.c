
#include "random.h"

#define N (624)                              // length of state vector
#define M (397)                              // a period parameter
#define K (0x9908B0DFU)                      // a magic constant
#define hiBit(u) ((u)&0x80000000U)           // mask all but highest   bit of u
#define loBit(u) ((u)&0x00000001U)           // mask all but lowest    bit of u
#define loBits(u) ((u)&0x7FFFFFFFU)          // mask     the highest   bit of u
#define mixBits(u, v) (hiBit(u) | loBits(v)) // move hi bit of u to hi bit of v

static u32 rnd_state[N + 1]; // state vector + 1 extra to not violate ANSI C
static u32 *next;            // next random value is computed from here
static int left = -1;        // can *next++ this many times before reloading

void seed_rand(u32 seed) {
    register u32 x = (seed | 1U) & 0xFFFFFFFFU, *s = rnd_state;
    register int j;
    for (left = 0, *s++ = x, j = N; --j;
         *s++ = (x *= 69069U) & 0xFFFFFFFFU);
}

internal u32 reload_MT(void) {
    register u32 *p0 = rnd_state, *p2 = rnd_state + 2, *pM = rnd_state + M, s0, s1;
    register int j;

    if (left < -1)
        seed_rand(4357U);

    left = N - 1, next = rnd_state + 1;

    for (s0 = rnd_state[0], s1 = rnd_state[1], j = N - M + 1; --j; s0 = s1, s1 = *p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

    for (pM = rnd_state, j = M; --j; s0 = s1, s1 = *p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);


    s1 = rnd_state[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
    s1 ^= (s1 >> 11);
    s1 ^= (s1 << 7) & 0x9D2C5680U;
    s1 ^= (s1 << 15) & 0xEFC60000U;
    return (s1 ^ (s1 >> 18));
}


u32 random_mersenne(void) {
    u32 y;

    if (--left < 0)
        return (reload_MT());

    y = *next++;
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9D2C5680U;
    y ^= (y << 15) & 0xEFC60000U;
    return (y ^ (y >> 18));
}
