#include <windows.h>

// Max number of possible ssh keys to dump.
// To avoid usage of linked list (lazy)
#define MAX_KEY_FILES 30

// Simple structure representing a ssh
typedef struct sshkey {
  PWSTR publicKeyPath;
  PWSTR secretKeyPath;
} sshkey;

void find_ssh_key(const PWSTR);