import socket
import os
import time

def savefile(username, datatype, filename, client):
    try :
        os.mkdir(username)
    except FileExistsError:
        print(f"DEBUG: {username} directory already exists.")
    except:
        print(f"DEBUG: Error while creating the {username} directory. Abort")
        return
    
    path = username + "/" + datatype
    try :
        os.mkdir(path)
    except FileExistsError:
        print(f"DEBUG: {path} directory already exists.")
    except:
        print(f"DEBUG: Error while creating the {path} directory. Abort")
        return

    path = username + "/" + datatype + "/" + str(int(time.time())) + "_" + filename
    fic = open(path, "w")
    ch = client.recv(9).decode("utf-8")
    while ch[-9:] != "[RUEPIHC]":
        try:
            cur_c = client.recv(1).decode("utf-8")
            ch += cur_c
        except:
            fic.write(ch)
            fic.close()
    ch = ch[:-9]
    print(f"DEBUG: Writing '{path}' file")
    fic.write(ch)
    fic.close()



def read_until_next_pipe_UTF16(client):
    ch = ""
    cur_c = ""
    while cur_c != "|":
        cur_c = client.recv(2).decode("utf-16")
        ch += cur_c
    return ch[:-1]

def start_server(host='0.0.0.0', port=1234):
    # Create a socket object
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Bind the socket to the address and port
    server_socket.bind((host, port))

    # Listen for incoming connections
    server_socket.listen(1)
    print(f"Listening on {host}:{port}...")

    while True:
        # Accept a connection
        client_socket, addr = server_socket.accept()
        print(f"Connection from {addr}")

        try:
            while True:
                chipeur = read_until_next_pipe_UTF16(client_socket)
                if chipeur != "[CHIPEUR]":
                    print("pas chipeur")
                    client_socket.close()

                username = read_until_next_pipe_UTF16(client_socket)

                datatype = read_until_next_pipe_UTF16(client_socket)

                if datatype == "file":
                    filename = read_until_next_pipe_UTF16(client_socket)
                    savefile(username, datatype, filename, client_socket)
        except ConnectionResetError:
            print("Client closed the connection")
            break


if __name__ == "__main__":
    start_server()
