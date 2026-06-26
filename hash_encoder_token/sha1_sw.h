#ifndef SHA1_SW_H
#define SHA1_SW_H

#include <stddef.h>
#include <stdint.h>

#define SHA1_DIGEST_WORDS 5

void sha1_sw(const uint8_t *data, size_t length,
	     uint32_t digest[SHA1_DIGEST_WORDS]);

#endif
