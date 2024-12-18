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
  hFile = CreateFile(filename,               // Nom du fichier
                     GENERIC_READ,           // Accès en lecture
                     0,                      // Mode de partage (aucun)
                     NULL,                   // Sécurité (par défaut)
                     OPEN_EXISTING,          //
                     FILE_ATTRIBUTE_NORMAL,  // Attributs du fichier
                     NULL  // Handle de fichier de modèle (aucun)
  );

  if (hFile == INVALID_HANDLE_VALUE) {
    printf("[Error] print_file: couldn't open %s file.\n", filename);
    return;
  }

  // Lire le fichier par blocs de 1024 octets
  do {
    result = ReadFile(hFile,           // Handle du fichier
                      buffer,          // Tampon de lecture
                      sizeof(buffer),  // Taille du tampon
                      &bytesRead,      // Nombre d'octets lus
                      NULL             // Structure OVERLAPPED (aucune)
    );

    if (!result || bytesRead == 0) {
      break;
    }

    // Afficher le contenu lu
    printf("%.*s", bytesRead, buffer);

  } while (bytesRead > 0);

  CloseHandle(hFile);
}