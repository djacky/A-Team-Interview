#include "EncryptSSL.h"

extern "C" {
    #define UI UI_OpenSSL
    #include <openssl/rsa.h>
    #include <openssl/pem.h>
    #include <openssl/aes.h>
    #include <openssl/rand.h>
    #include <openssl/evp.h>
    #undef UI
}

#include "Misc/Base64.h" // Unreal's Base64 encoding

std::vector<std::string> EncryptKeys(const std::string& CryptoKeys, const std::string& PublicKey)
{
    std::vector<std::string> result;

    // Convert the public key from string to a format OpenSSL can use
    BIO* Bio = BIO_new_mem_buf(PublicKey.c_str(), -1);
    RSA* RsaPublicKey = PEM_read_bio_RSA_PUBKEY(Bio, NULL, NULL, NULL);
    BIO_free(Bio);

    if (!RsaPublicKey)
    {
        return result;
    }

    // Generate AES key
    uint8_t AesKey[32];
    if (!RAND_bytes(AesKey, sizeof(AesKey)))
    {
        RSA_free(RsaPublicKey);
        return result;
    }

    // Encrypt the cryptoKeys using AES
    uint8_t Iv[AES_BLOCK_SIZE];
    if (!RAND_bytes(Iv, sizeof(Iv)))
    {
        RSA_free(RsaPublicKey);
        return result;
    }

    EVP_CIPHER_CTX* EncryptCtx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(EncryptCtx, EVP_aes_256_cbc(), NULL, AesKey, Iv);

    std::vector<uint8_t> CipherText(CryptoKeys.size() + AES_BLOCK_SIZE);
    int CipherTextLen = 0;
    int Len = 0;

    EVP_EncryptUpdate(EncryptCtx, CipherText.data(), &Len, reinterpret_cast<const uint8_t*>(CryptoKeys.data()), CryptoKeys.size());
    CipherTextLen = Len;

    EVP_EncryptFinal_ex(EncryptCtx, CipherText.data() + Len, &Len);
    CipherTextLen += Len;
    CipherText.resize(CipherTextLen);

    EVP_CIPHER_CTX_free(EncryptCtx);

    // Encrypt the AES key using the public RSA key
    int EncryptedAesKeyLen = RSA_size(RsaPublicKey);
    std::vector<uint8_t> EncryptedAesKey(EncryptedAesKeyLen);

    EncryptedAesKeyLen = RSA_public_encrypt(sizeof(AesKey), AesKey, EncryptedAesKey.data(), RsaPublicKey, RSA_PKCS1_OAEP_PADDING);
    RSA_free(RsaPublicKey);

    if (EncryptedAesKeyLen == -1)
    {
        return result;
    }

    // Base64 encode the results
    FString EncodedAesKey = FBase64::Encode(EncryptedAesKey.data(), EncryptedAesKeyLen);
    FString EncodedIv = FBase64::Encode(Iv, sizeof(Iv));
    FString EncodedCipherText = FBase64::Encode(CipherText.data(), CipherText.size());

    // Convert FString to std::string and push back into the result vector
    result.push_back(std::string(TCHAR_TO_UTF8(*EncodedAesKey)));
    result.push_back(std::string(TCHAR_TO_UTF8(*EncodedIv)));
    result.push_back(std::string(TCHAR_TO_UTF8(*EncodedCipherText)));

    return result;
}
