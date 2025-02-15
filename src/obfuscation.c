#include "obfuscation.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>
/**
 * XOR the given string pointer by xoring each char with 42
 * @param str : the string to obfuscate/deobfuscate by xoring each character
 * @param size : size of given string
 */
void xor_str(char *str, int size) {
  while (size-- > 0) {
    *str++ ^= 42;
  }
}

/**
 * XOR the given wide string pointer by xoring each wide char with 42
 * @param wstr : the wide string to obfuscate/deobfuscate by xoring each wide
 * character
 * @param size : size of given string
 */
void xor_wstr(wchar_t *wstr, int size) {
  while (size-- > 0) {
    *wstr++ ^= 42;
  }
}

void resolve_apis(hidden_apis *apis) {
  wchar_t kernel_str[] =
      L"\x41\x4f\x58\x44\x4f\x46\x19\x18\x04\x4e\x46\x46";  // kernel32.dll
  XOR_WSTR(kernel_str, wcslen(kernel_str));

  HMODULE hKernel32 = GetModuleHandleW(kernel_str);

  char checkRemoteDbg_str[] =
      "\x69\x42\x4f\x49\x41\x78\x4f\x47\x45\x5e\x4f\x6e\x4f\x48\x5f\x4d\x4d\x4f"
      "\x58\x7a\x58\x4f\x59\x4f\x44\x5e";  // CheckRemoteDebuggerPresent
  XOR_STR(checkRemoteDbg_str, strlen(checkRemoteDbg_str));

  apis->funcCheckRemoteDebuggerPresent =
      (PCheckRemoteDebuggerPresent)GetProcAddress(hKernel32, checkRemoteDbg_str);
}
