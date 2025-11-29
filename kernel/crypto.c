#include "crypto.h"

static uint8_t g_key[32];
static size_t  g_key_len = 0;

void crypto_set_key(const char* key)
{
    g_key_len = 0;
    while (key[g_key_len] && g_key_len < sizeof(g_key)) {
        g_key[g_key_len] = (uint8_t)key[g_key_len];
        g_key_len++;
    }
    if (g_key_len == 0) {
        /* fallback key so we never have 0-length */
        g_key[0] = 0x42;
        g_key_len = 1;
    }
}

static void crypto_apply(const uint8_t* in, uint8_t* out, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        out[i] = in[i] ^ g_key[i % g_key_len];
    }
}

void crypto_encrypt(const uint8_t* in, uint8_t* out, size_t len)
{
    crypto_apply(in, out, len);
}

void crypto_decrypt(const uint8_t* in, uint8_t* out, size_t len)
{
    crypto_apply(in, out, len);
}
    