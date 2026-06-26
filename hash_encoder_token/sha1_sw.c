#include "sha1_sw.h"

#include <string.h>

static uint32_t rotate_left(uint32_t value, unsigned int bits)
{
	return (value << bits) | (value >> (32U - bits));
}

static void sha1_transform(uint32_t state[SHA1_DIGEST_WORDS],
			   const uint8_t block[64])
{
	uint32_t words[80];
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint32_t e;

	for (size_t i = 0; i < 16; i++) {
		words[i] = ((uint32_t)block[(i * 4) + 0] << 24) |
			   ((uint32_t)block[(i * 4) + 1] << 16) |
			   ((uint32_t)block[(i * 4) + 2] << 8) |
			   ((uint32_t)block[(i * 4) + 3]);
	}

	for (size_t i = 16; i < 80; i++) {
		words[i] = rotate_left(words[i - 3] ^ words[i - 8] ^
					      words[i - 14] ^ words[i - 16],
				      1);
	}

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];

	for (size_t i = 0; i < 80; i++) {
		uint32_t function;
		uint32_t constant;
		uint32_t temporary;

		if (i < 20) {
			function = (b & c) | ((~b) & d);
			constant = 0x5a827999;
		} else if (i < 40) {
			function = b ^ c ^ d;
			constant = 0x6ed9eba1;
		} else if (i < 60) {
			function = (b & c) | (b & d) | (c & d);
			constant = 0x8f1bbcdc;
		} else {
			function = b ^ c ^ d;
			constant = 0xca62c1d6;
		}

		temporary = rotate_left(a, 5) + function + e + constant +
			    words[i];
		e = d;
		d = c;
		c = rotate_left(b, 30);
		b = a;
		a = temporary;
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
}

void sha1_sw(const uint8_t *data, size_t length,
	     uint32_t digest[SHA1_DIGEST_WORDS])
{
	uint32_t state[SHA1_DIGEST_WORDS] = {
		0x67452301,
		0xefcdab89,
		0x98badcfe,
		0x10325476,
		0xc3d2e1f0,
	};
	uint8_t final_blocks[128] = { 0 };
	size_t full_blocks = length / 64;
	size_t remaining = length % 64;
	size_t final_length;
	uint64_t bit_length = (uint64_t)length * 8;

	for (size_t i = 0; i < full_blocks; i++) {
		sha1_transform(state, data + (i * 64));
	}

	memcpy(final_blocks, data + (full_blocks * 64), remaining);
	final_blocks[remaining] = 0x80;
	final_length = remaining < 56 ? 64 : 128;

	for (size_t i = 0; i < 8; i++) {
		final_blocks[final_length - 1 - i] =
			(uint8_t)(bit_length >> (i * 8));
	}

	sha1_transform(state, final_blocks);
	if (final_length == 128) {
		sha1_transform(state, final_blocks + 64);
	}

	memcpy(digest, state, sizeof(state));
}
