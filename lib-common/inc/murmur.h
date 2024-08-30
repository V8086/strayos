#ifndef MURMUR_H
#define MURMUR_H										/* Murmur hash */

#include <stdint.h>
#include <string.h>


/* https://en.wikipedia.org/wiki/MurmurHash#Algorithm */
static inline uint32_t MurmurScrambleDword(uint32_t k) {
	k *= 0xcc9e2d51;
	k = (k << 15) | (k >> 17);
	k *= 0x1b873593;
	return k;
}

uint32_t Murmur3Dword(const uint8_t* key, size_t len, uint32_t seed);

#endif