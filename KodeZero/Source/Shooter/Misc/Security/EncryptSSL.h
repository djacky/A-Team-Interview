#pragma once

#include <vector>
#include <string>

// Forward declare OpenSSL types to avoid including OpenSSL headers directly
struct rsa_st;
typedef struct rsa_st RSA;

std::vector<std::string> EncryptKeys(const std::string& CryptoKeys, const std::string& PublicKey);
