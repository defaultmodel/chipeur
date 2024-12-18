#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "extract_file.h"

static void find_key_pair(const char *directory, const char *pkName, size_t lenPkFile){
    char * skName = malloc(sizeof(char) * lenPkFile - 3);
    if (skName == NULL){
        printf("[Error] find_key_pair: malloc for skName failed.\n");
        return;
    }

    char skNamePath[MAX_PATH];
    char pkNamePath[MAX_PATH];

    // Copying the public key file into secret key
    // And removing the .pub
    strncpy(skName, pkName, lenPkFile - 4);
    skName[lenPkFile - 4] = '\0';

    snprintf(skNamePath, sizeof(skNamePath), "%s\\%s", directory, skName);
    snprintf(pkNamePath, sizeof(pkNamePath), "%s\\%s", directory, pkName);


    DWORD attributes = GetFileAttributesA(skNamePath);
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)){
        printf("[ERROR] find_key_pair: couldn't find private key for %s\n", pkNamePath);

    }
    printf("skName = %s\n", skName);
    print_file(skNamePath);
    print_file(pkNamePath);
}

void find_ssh_key(const char *directory) {
  WIN32_FIND_DATA findData;
  HANDLE hFind;
  char searchPath[MAX_PATH];

  // Construire le chemin de recherche
  snprintf(searchPath, sizeof(searchPath), "%s\\*", directory);

  // Commencer la recherche
  hFind = FindFirstFile(searchPath, &findData);
  if (hFind == INVALID_HANDLE_VALUE) {
    // printf("[DEBUG] %s directory is empty.\n", directory);
    return;
  }

  // Looping on all the file
  do {
    const char *fileName = findData.cFileName;

    // Ignore "." and ".."
    if (strcmp(fileName, ".") == 0 || strcmp(fileName, "..") == 0) {
      continue;
    }

    // Check the filetype (file or directory)
    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      char subDirectory[MAX_PATH];
      snprintf(subDirectory, sizeof(subDirectory), "%s\\%s", directory,
               fileName);

      // Repeat the process on the subdirectories
      find_ssh_key(subDirectory);
    } else {
      size_t lenFileName = strlen(fileName);
      
      // checking if the end of the file is .pub
      // i.e. ssh public key
      if (strcmp(fileName + (lenFileName - 4), ".pub") == 0) {
        printf("[Fichier] %s\\%s\n", directory, fileName);
        find_key_pair(directory, fileName, lenFileName);
      }
    }
  } while (FindNextFile(hFind, &findData) != 0);

  FindClose(hFind);
}