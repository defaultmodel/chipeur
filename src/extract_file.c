#include "extract_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

void print_file(const PWSTR filename) {
  HANDLE hFile;
  DWORD bytesRead;
  char buffer[1024];  // Size of the buffer
  BOOL result;

  // Open the file in reading mode
  hFile = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
#ifdef DEBUG
    wprintf(L"[Error] print_file: couldn't open %ls file.\n", filename);
#endif
    return;
  }
#ifdef DEBUG
  wprintf(L"[Debug] print_file: Opening '%ls' file.\n", filename);
#endif
  fflush(stdout);
  // reading 1024 bytes block
  do {
    result = ReadFile(hFile,  // Handler
                      buffer, sizeof(buffer), &bytesRead, NULL);

    if (!result || bytesRead == 0) {
      break;
    }

    // Afficher le contenu lu
    printf("%.*s", bytesRead, buffer);

  } while (bytesRead > 0);

  CloseHandle(hFile);
}

BOOL is_readable(const PWSTR filename){
  // check if the file is readable.

  HANDLE hFile = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
#ifdef DEBUG
    wprintf(L"DEBUG: is_readable: couldn't read %ls file.\n", filename);
#endif
    return FALSE;
  }
  CloseHandle(hFile);
  return TRUE;
}