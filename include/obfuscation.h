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

typedef BOOL(WINAPI *PCheckRemoteDebuggerPresent)(HANDLE hProcess, PBOOL pbDebuggerPresent);
typedef struct {
  PCheckRemoteDebuggerPresent funcCheckRemoteDebuggerPresent;
} hidden_apis;
void resolve_apis(hidden_apis *apis);
