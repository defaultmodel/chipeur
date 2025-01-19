#include "obfuscation.h"
/**
 * XOR the given string pointer by xoring each chat with 42
 * @param str : the string to obfuscate/deobfuscate by xoring each character
 * @param size : size of given string
 */
void xor_str(char *str, int size) {
  while (size-- > 0) {
    *str++ ^= 42;
  }
}
