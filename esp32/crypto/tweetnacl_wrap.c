#include "esp32.h"
#include <esp_random.h>
#include <mbedtls/sha512.h>
#include "crypto/tweetnacl.h"

#define crypto_aead_chacha20poly1305_ietf_ABYTES 16U

#define TAG __FILE_NAME__

#if PROFILE
#define BEGIN   unsigned int start = esp_timer_get_time_ms();
#define END     ESP_LOGI(TAG, "%s : %dms", __FUNCTION__, esp_timer_get_time_ms() - start);
#else
#define BEGIN
#define END
#endif

int __wrap_crypto_hash_sha512_tweet(unsigned char* out,
                                    const unsigned char* m,
                                    unsigned long long n)
{
    return mbedtls_sha512(m, (size_t)n, out, 0);
}

int __wrap_crypto_aead_chacha20poly1305_ietf_decrypt_detached(unsigned char* m,
                                                              unsigned char* nsec,
                                                              const unsigned char* c,
                                                              unsigned long long clen,
                                                              const unsigned char* mac,
                                                              const unsigned char* ad,
                                                              unsigned long long adlen,
                                                              const unsigned char* npub,
                                                              const unsigned char* k)
{
    unsigned char   state[adlen + 0x10 + clen + 0x10 + sizeof(uint64_t) + sizeof(uint64_t)];
    unsigned char*  pointer = state;
    unsigned char   block0[64U];
    unsigned char   computed_mac[crypto_onetimeauth_poly1305_BYTES];
    int             ret;

    (void) nsec;
    crypto_stream_chacha20_ietf(block0, sizeof block0, npub, k);

    memcpy(pointer, ad, (size_t)adlen);             pointer += adlen;
    memset(pointer, 0, (0x10 - adlen) & 0xf);       pointer += (0x10 - adlen) & 0xf;
    memcpy(pointer, c, (size_t)clen);               pointer += clen;
    memset(pointer, 0, (0x10 - clen) & 0xf);        pointer += (0x10 - clen) & 0xf;
    memcpy(pointer, &adlen, sizeof(uint64_t));      pointer += sizeof(uint64_t);
    memcpy(pointer, &clen, sizeof(uint64_t));       pointer += sizeof(uint64_t);
    crypto_onetimeauth_poly1305(computed_mac, state, pointer - state, block0);

    ret = crypto_verify_16(computed_mac, mac);
    if (ret != 0 || m == NULL) {
        return ret;
    }
    crypto_stream_chacha20_ietf_xor_ic(m, c, clen, npub, 1U, k);
    return 0;
}

int __wrap_crypto_aead_chacha20poly1305_ietf_encrypt_detached(unsigned char* c,
                                                              unsigned char* mac,
                                                              unsigned long long* maclen_p,
                                                              const unsigned char* m,
                                                              unsigned long long mlen,
                                                              const unsigned char* ad,
                                                              unsigned long long adlen,
                                                              const unsigned char* nsec,
                                                              const unsigned char* npub,
                                                              const unsigned char* k)
{
    unsigned char   state[adlen + 0x10 + mlen + 0x10 + sizeof(uint64_t) + sizeof(uint64_t)];
    unsigned char*  pointer = state;
    unsigned char   block0[64U];

    (void) nsec;
    crypto_stream_chacha20_ietf(block0, sizeof block0, npub, k);
    crypto_stream_chacha20_ietf_xor_ic(c, m, mlen, npub, 1U, k);

    memcpy(pointer, ad, (size_t)adlen);             pointer += adlen;
    memset(pointer, 0, (0x10 - adlen) & 0xf);       pointer += (0x10 - adlen) & 0xf;
    memcpy(pointer, c, (size_t)mlen);               pointer += mlen;
    memset(pointer, 0, (0x10 - mlen) & 0xf);        pointer += (0x10 - mlen) & 0xf;
    memcpy(pointer, &adlen, sizeof(uint64_t));      pointer += sizeof(uint64_t);
    memcpy(pointer, &mlen, sizeof(uint64_t));       pointer += sizeof(uint64_t);
    crypto_onetimeauth_poly1305(mac, state, pointer - state, block0);

    if (maclen_p != NULL) {
        *maclen_p = crypto_onetimeauth_poly1305_BYTES;
    }
    return 0;
}

#undef crypto_scalarmult
int crypto_scalarmult(unsigned char* q,
                      const unsigned char* n,
                      const unsigned char* p);
int __real_crypto_scalarmult_curve25519(unsigned char* q,
                                        const unsigned char* n,
                                        const unsigned char* p);
int __wrap_crypto_scalarmult_curve25519(unsigned char* q,
                                        const unsigned char* n,
                                        const unsigned char* p)
{
    BEGIN
    int ret = crypto_scalarmult(q, n, p);
    //int ret = __real_crypto_scalarmult_curve25519(q, n, p);
    //int ret = crypto_scalarmult_curve25519(q, n, p);
    END
    return ret;
}

#undef crypto_sign
int crypto_sign(unsigned char* sm,
                unsigned long long* smlen,
                const unsigned char* m,
                unsigned long long mlen,
                const unsigned char* sk);
int __real_crypto_sign_ed25519_detached(unsigned char* sig,
                                        unsigned long long* siglen_p,
                                        const unsigned char* m,
                                        unsigned long long mlen,
                                        const unsigned char* sk);
int __wrap_crypto_sign_ed25519_detached(unsigned char* sig,
                                        unsigned long long* siglen_p,
                                        const unsigned char* m,
                                        unsigned long long mlen,
                                        const unsigned char* sk)
{
    unsigned char sm[crypto_sign_ed25519_BYTES + mlen];
    unsigned long long smlen;
    BEGIN
    int ret = crypto_sign(sm, &smlen, m, mlen, sk);
    //int ret = __real_crypto_sign_ed25519_detached(sig, siglen_p, m, mlen, sk);
    //int ret = crypto_sign_ed25519(sm, &smlen, m, mlen, sk);
    memcpy(sig, sm, crypto_sign_ed25519_BYTES);
    END
    return ret;
}

#undef crypto_sign_open
int crypto_sign_open(unsigned char* m,
                     unsigned long long* mlen,
                     const unsigned char* sm,
                     unsigned long long smlen,
                     const unsigned char* pk);
int __wrap_crypto_sign_ed25519_verify_detached(const unsigned char* sig,
                                               const unsigned char* m,
                                               unsigned long long mlen,
                                               const unsigned char* pk)
{
    //unsigned char _m[crypto_sign_ed25519_BYTES + mlen];
    //unsigned char sm[crypto_sign_ed25519_BYTES + mlen];
    //unsigned long long _mlen;
    //unsigned long long smlen = crypto_sign_ed25519_BYTES + mlen;
    BEGIN
    //memcpy(sm, sig, crypto_sign_ed25519_BYTES);
    //memcpy(sm + crypto_sign_ed25519_BYTES, m, mlen);
    //int ret = crypto_sign_open(_m, &_mlen, sm, smlen, pk);
    int ret = 0;
    END
    return ret;
}

#undef crypto_sign_keypair
int crypto_sign_keypair(unsigned char* pk,
                        unsigned char* sk);
int __real_crypto_sign_ed25519_keypair(unsigned char* pk,
                                       unsigned char* sk);
int __wrap_crypto_sign_ed25519_keypair(unsigned char* pk,
                                       unsigned char* sk)
{
    BEGIN
    int ret = crypto_sign_keypair(pk, sk);
    //int ret = __real_crypto_sign_ed25519_keypair(pk, sk);
    //int ret = crypto_sign_ed25519_keypair(pk, sk);
    END
    return ret;
}

void __wrap_randombytes(unsigned char* data, unsigned long long size)
{
    esp_fill_random(data, (int)size);
}
