#pragma once
#include <stddef.h>
#include <stdint.h>

void crypto_set_key(const char* key);

void crypto_encrypt(const uint8_t* in, uint8_t* out, size_t len);
void crypto_decrypt(const uint8_t* in, uint8_t* out, size_t len);
