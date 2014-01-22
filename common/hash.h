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

#endif /// _MERRY_HASH_H

/* Notice extracted from xxHash homepage :

xxHash is an extremely fast Hash algorithm, running at RAM speed limits.
It also successfully passes all tests from the SMHasher suite.

Comparison (single thread, Windows Seven 32 bits, using SMHasher on a Core 2 Duo @3GHz)

Name            Speed       Q.Score   Author
xxHash          5.4 GB/s     10
CrapWow         3.2 GB/s      2       Andrew
MumurHash 3a    2.7 GB/s     10       Austin Appleby
SpookyHash      2.0 GB/s     10       Bob Jenkins
SBox            1.4 GB/s      9       Bret Mulvey
Lookup3         1.2 GB/s      9       Bob Jenkins
SuperFastHash   1.2 GB/s      1       Paul Hsieh
CityHash64      1.05 GB/s    10       Pike & Alakuijala
FNV             0.55 GB/s     5       Fowler, Noll, Vo
CRC32           0.43 GB/s     9
MD5-32          0.33 GB/s    10       Ronald L. Rivest
SHA1-32         0.28 GB/s    10

Q.Score is a measure of quality of the hash function. 
It depends on successfully passing SMHasher test set. 
10 is a perfect score.
*/

#pragma once

#if defined (__cplusplus)
extern "C" {
#endif


//****************************
// Type
//****************************
typedef enum { XXH_OK=0, XXH_ERROR } XXH_errorcode;



//****************************
// Simple Hash Functions
//****************************

unsigned int XXH32 (const void* input, int len, unsigned int seed);

/*
XXH32() :
    Calculate the 32-bits hash of sequence of length "len" stored at memory address "input".
    The memory between input & input+len must be valid (allocated and read-accessible).
    "seed" can be used to alter the result predictably.
    This function successfully passes all SMHasher tests.
    Speed on Core 2 Duo @ 3 GHz (single thread, SMHasher benchmark) : 5.4 GB/s
    Note that "len" is type "int", which means it is limited to 2^31-1.
    If your data is larger, use the advanced functions below.
*/



//****************************
// Advanced Hash Functions
//****************************

void*         XXH32_init   (unsigned int seed);
XXH_errorcode XXH32_update (void* state, const void* input, int len);
unsigned int  XXH32_digest (void* state);

/*
These functions calculate the xxhash of an input provided in several small packets,
as opposed to an input provided as a single block.

It must be started with :
void* XXH32_init()
The function returns a pointer which holds the state of calculation.

This pointer must be provided as "void* state" parameter for XXH32_update().
XXH32_update() can be called as many times as necessary.
The user must provide a valid (allocated) input.
The function returns an error code, with 0 meaning OK, and any other value meaning there is an error.
Note that "len" is type "int", which means it is limited to 2^31-1. 
If your data is larger, it is recommended to chunk your data into blocks 
of size for example 2^30 (1GB) to avoid any "int" overflow issue.

Finally, you can end the calculation anytime, by using XXH32_digest().
This function returns the final 32-bits hash.
You must provide the same "void* state" parameter created by XXH32_init().
Memory will be freed by XXH32_digest().
*/


int           XXH32_sizeofState();
XXH_errorcode XXH32_resetState(void* state, unsigned int seed);

#define       XXH32_SIZEOFSTATE 48
typedef struct { long long ll[(XXH32_SIZEOFSTATE+(sizeof(long long)-1))/sizeof(long long)]; } XXH32_stateSpace_t;
/*
These functions allow user application to make its own allocation for state.

XXH32_sizeofState() is used to know how much space must be allocated for the xxHash 32-bits state.
Note that the state must be aligned to access 'long long' fields. Memory must be allocated and referenced by a pointer.
This pointer must then be provided as 'state' into XXH32_resetState(), which initializes the state.

For static allocation purposes (such as allocation on stack, or freestanding systems without malloc()),
use the structure XXH32_stateSpace_t, which will ensure that memory space is large enough and correctly aligned to access 'long long' fields.
*/


unsigned int XXH32_intermediateDigest (void* state);
/*
This function does the same as XXH32_digest(), generating a 32-bit hash,
but preserve memory context.
This way, it becomes possible to generate intermediate hashes, and then continue feeding data with XXH32_update().
To free memory context, use XXH32_digest(), or free().
*/



//****************************
// Deprecated function names
//****************************
// The following translations are provided to ease code transition
// You are encouraged to no longer this function names
#define XXH32_feed   XXH32_update
#define XXH32_result XXH32_digest
#define XXH32_getIntermediateResult XXH32_intermediateDigest



#if defined (__cplusplus)
}
#endif
