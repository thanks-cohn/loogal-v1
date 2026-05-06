#include "loogal/sha256.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    unsigned char data[64];
    size_t datalen;
} LoogalSha256Ctx;

static uint32_t rotr32(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32U - n));
}

static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t ep0(uint32_t x) {
    return rotr32(x, 2) ^ rotr32(x, 13) ^ rotr32(x, 22);
}

static uint32_t ep1(uint32_t x) {
    return rotr32(x, 6) ^ rotr32(x, 11) ^ rotr32(x, 25);
}

static uint32_t sig0(uint32_t x) {
    return rotr32(x, 7) ^ rotr32(x, 18) ^ (x >> 3);
}

static uint32_t sig1(uint32_t x) {
    return rotr32(x, 17) ^ rotr32(x, 19) ^ (x >> 10);
}

static const uint32_t k[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

static void sha256_transform(LoogalSha256Ctx *ctx, const unsigned char data[64]) {
    uint32_t m[64];

    for (int i = 0; i < 16; i++) {
        m[i] =
            ((uint32_t)data[i * 4] << 24) |
            ((uint32_t)data[i * 4 + 1] << 16) |
            ((uint32_t)data[i * 4 + 2] << 8) |
            ((uint32_t)data[i * 4 + 3]);
    }

    for (int i = 16; i < 64; i++) {
        m[i] = sig1(m[i - 2]) + m[i - 7] + sig0(m[i - 15]) + m[i - 16];
    }

    uint32_t a = ctx->state[0];
    uint32_t b = ctx->state[1];
    uint32_t c = ctx->state[2];
    uint32_t d = ctx->state[3];
    uint32_t e = ctx->state[4];
    uint32_t f = ctx->state[5];
    uint32_t g = ctx->state[6];
    uint32_t h = ctx->state[7];

    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + ep1(e) + ch(e, f, g) + k[i] + m[i];
        uint32_t t2 = ep0(a) + maj(a, b, c);

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void sha256_init(LoogalSha256Ctx *ctx) {
    ctx->datalen = 0;
    ctx->bitlen = 0;

    ctx->state[0] = 0x6a09e667U;
    ctx->state[1] = 0xbb67ae85U;
    ctx->state[2] = 0x3c6ef372U;
    ctx->state[3] = 0xa54ff53aU;
    ctx->state[4] = 0x510e527fU;
    ctx->state[5] = 0x9b05688cU;
    ctx->state[6] = 0x1f83d9abU;
    ctx->state[7] = 0x5be0cd19U;
}

static void sha256_update(LoogalSha256Ctx *ctx, const unsigned char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        ctx->data[ctx->datalen++] = data[i];

        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(LoogalSha256Ctx *ctx, unsigned char hash[32]) {
    size_t i = ctx->datalen;

    ctx->data[i++] = 0x80;

    if (i > 56) {
        while (i < 64) {
            ctx->data[i++] = 0x00;
        }

        sha256_transform(ctx, ctx->data);
        i = 0;
    }

    while (i < 56) {
        ctx->data[i++] = 0x00;
    }

    ctx->bitlen += (uint64_t)ctx->datalen * 8ULL;

    ctx->data[63] = (unsigned char)(ctx->bitlen);
    ctx->data[62] = (unsigned char)(ctx->bitlen >> 8);
    ctx->data[61] = (unsigned char)(ctx->bitlen >> 16);
    ctx->data[60] = (unsigned char)(ctx->bitlen >> 24);
    ctx->data[59] = (unsigned char)(ctx->bitlen >> 32);
    ctx->data[58] = (unsigned char)(ctx->bitlen >> 40);
    ctx->data[57] = (unsigned char)(ctx->bitlen >> 48);
    ctx->data[56] = (unsigned char)(ctx->bitlen >> 56);

    sha256_transform(ctx, ctx->data);

    for (int j = 0; j < 4; j++) {
        hash[j]      = (unsigned char)((ctx->state[0] >> (24 - j * 8)) & 0xff);
        hash[j + 4]  = (unsigned char)((ctx->state[1] >> (24 - j * 8)) & 0xff);
        hash[j + 8]  = (unsigned char)((ctx->state[2] >> (24 - j * 8)) & 0xff);
        hash[j + 12] = (unsigned char)((ctx->state[3] >> (24 - j * 8)) & 0xff);
        hash[j + 16] = (unsigned char)((ctx->state[4] >> (24 - j * 8)) & 0xff);
        hash[j + 20] = (unsigned char)((ctx->state[5] >> (24 - j * 8)) & 0xff);
        hash[j + 24] = (unsigned char)((ctx->state[6] >> (24 - j * 8)) & 0xff);
        hash[j + 28] = (unsigned char)((ctx->state[7] >> (24 - j * 8)) & 0xff);
    }
}

static void hash_to_hex(const unsigned char hash[32], char out_hex[65]) {
    static const char *hex = "0123456789abcdef";

    for (int i = 0; i < 32; i++) {
        out_hex[i * 2] = hex[(hash[i] >> 4) & 0x0f];
        out_hex[i * 2 + 1] = hex[hash[i] & 0x0f];
    }

    out_hex[64] = '\0';
}

int loogal_sha256_bytes_hex(const unsigned char *data, size_t len, char out_hex[65]) {
    if (!data && len > 0) return -1;
    if (!out_hex) return -1;

    LoogalSha256Ctx ctx;
    unsigned char hash[32];

    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
    hash_to_hex(hash, out_hex);

    return 0;
}

int loogal_sha256_file_hex(const char *path, char out_hex[65]) {
    if (!path || !out_hex) return -1;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    LoogalSha256Ctx ctx;
    unsigned char buf[65536];
    unsigned char hash[32];

    sha256_init(&ctx);

    for (;;) {
        size_t n = fread(buf, 1, sizeof(buf), f);

        if (n > 0) {
            sha256_update(&ctx, buf, n);
        }

        if (n < sizeof(buf)) {
            if (ferror(f)) {
                fclose(f);
                return -1;
            }

            break;
        }
    }

    fclose(f);

    sha256_final(&ctx, hash);
    hash_to_hex(hash, out_hex);

    return 0;
}


int loogal_sha256_file(const char *path, char out_hex[65]) {
    return loogal_sha256_file_hex(path, out_hex);
}
