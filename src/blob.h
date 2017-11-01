#ifndef SRC_BLOB_H
#define SRC_BLOB_H

#include <stddef.h>
#include <stdint.h>

struct LITE_BIGNUM {
	uint32_t dmax;              /* Size of d, in 32-bit words. */
	uint32_t d;  /* Word array, little endian format ... */
} __attribute__((packed));

struct LITE_RSA {
	uint32_t e;
	struct LITE_BIGNUM N;
	struct LITE_BIGNUM d;
} __attribute__((packed));


/* TODO: maybe use protos? */
struct blob_enforcements {
	uint32_t tag;
	uint32_t value;  /* TODO: may need buffer here. */
} __attribute__((packed));

struct blob_rsa {
	struct  LITE_RSA rsa;
	uint8_t N_bytes[4096 >> 3];
	uint8_t d_bytes[4096 >> 3];
} __attribute__((packed));

struct blob_ec {
	uint32_t curve;
	/* TODO: up this size once 384, 521 support is done. */
	uint8_t d[32];
	uint8_t x[32];
	uint8_t y[32];
} __attribute__((packed));

struct blob_sym {
	/* TODO: max HMAC key size? */
	uint8_t bytes[32];
} __attribute__((packed));

enum blob_alg : uint32_t {
	BLOB_RSA = 1,
	BLOB_EC = 2,
	BLOB_AES = 3,
	BLOB_DES = 4,
	BLOB_HMAC = 5,
};

#define KM_BLOB_MAGIC   0x474F4F47
#define KM_BLOB_VERSION 1

struct km_blob {
	struct {
		uint32_t id;
		/* TODO: salt etc. */
	} header __attribute__((packed));
	struct {
		uint32_t magic;
		uint32_t version;
		struct blob_enforcements enforced[16];
		enum blob_alg algorithm;
		union {
			struct blob_rsa rsa;
			struct blob_ec  ec;
			struct blob_sym sym;
		} key;
		/* TODO: pad to block size. */
	} b __attribute__((packed));
	uint8_t hmac[32];
} __attribute__((packed));

#endif  // SRC_BLOB_H
