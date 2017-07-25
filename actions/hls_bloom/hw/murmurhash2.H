
#ifndef _BLOOM_MURMURHASH2
#define _BLOOM_MURMURHASH2

//unsigned int murmurhash2(const void * key, int len, const unsigned int seed);
unsigned int murmurhash2(const char key[32], int len, const unsigned int seed);

#endif
