#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "extract_file.h"

void print_file(const char* filename) {
  HANDLE hFile;
  DWORD bytesRead;
  char buffer[1024];  // Size of the buffer
  BOOL result;

  // Open the file in reading mode
  hFile = CreateFile(filename, GENERIC_READ, 0, NULL,\
                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    printf("[Error] print_file: couldn't open %s file.\n", filename);
    return;
  }

  // reading 1024 bytes block
  do {
    result = ReadFile(hFile,           // Handler
                      buffer,
                      sizeof(buffer),
                      &bytesRead,
                      NULL
    );

    if (!result || bytesRead == 0) {
      break;
    }

    // Afficher le contenu lu
    printf("%.*s", bytesRead, buffer);

  } while (bytesRead > 0);

  CloseHandle(hFile);
}