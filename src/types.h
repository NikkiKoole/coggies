#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>  // int8_t et al.
#include <stdbool.h> // bool
#include <stdlib.h>  // size_t
#include <stdio.h>

#define PI 3.14159265359
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CLAMP(x, lo, hi) MIN((hi), MAX((lo), (x)))
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))
#define UNUSED(x) (void)(x)
#define ABS(x)(((x) < 0) ? -(x) : (x))

#define ASSERT(expression)                                                                               \
    if (!(expression)) {                                                                                 \
        printf("%s, function %s, file: %s, line:%d. \n", #expression, __FUNCTION__, __FILE__, __LINE__); \
        exit(0);                                                                                         \
    }

#define internal static
#define global_value

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;
typedef bool b32;

#endif
