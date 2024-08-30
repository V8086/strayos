#include <murmur.h>

/* https://en.wikipedia.org/wiki/MurmurHash#Algorithm */
uint32_t Murmur3Dword(const uint8_t* key, size_t len, uint32_t seed) {
	uint32_t h = seed;
	uint32_t k;

	for (size_t i = len >> 2; i; i--) {
		memcpy(&k, key, sizeof(uint32_t));
		key += sizeof(uint32_t);
		h ^= MurmurScrambleDword(k);
		h = (h << 13) | (h >> 19);
		h = h * 5 + 0xe6546b64;
	}

	k = 0;
	for (size_t i = len & 3; i; i--) {
		k <<= 8;
		k |= key[i - 1];
	}

	h ^= MurmurScrambleDword(k);

	h ^= (uint32_t)len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}