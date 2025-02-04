#include <stddef.h>

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

//void xor_str(char *str, int size);
//void xor_wstr(wchar_t *wstr, int size);
