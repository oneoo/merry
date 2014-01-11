#ifndef _MERRY_HASH_H
#define _MERRY_HASH_H

#include <stdint.h>

uint32_t fnv1a_32(const unsigned char *data, uint32_t len);
uint32_t fnv1a_64(const unsigned char *data, uint32_t len);

#if defined(__x86_64__)
#define MURMUR_HASH MurmurHash64A
uint64_t MurmurHash64A(const void *key, int len, unsigned int seed);
#define MurmurHash MurmurHash64A
typedef uint64_t murmur_t;

#elif defined(__i386__)
#define MURMUR_HASH MurmurHash2
unsigned int MurmurHash2(const void *key, int len, unsigned int seed);
#define MurmurHash MurmurHash2
typedef unsigned int murmur_t;

#else
#define MURMUR_HASH MurmurHashNeutral2
unsigned int MurmurHashNeutral2(const void *key, int len, unsigned int seed);
#define MurmurHash MurmurHashNeutral2
typedef unsigned int murmur_t;

#endif

#endif /* MURMURHASH_H */