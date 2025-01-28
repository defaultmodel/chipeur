import socket
import os

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

    path = username + "/" + datatype + "/" + filename
    fic = open(path, "w")
    ch = client.recv(9).decode()
    print(ch)
    while ch[-9:] != "[RUEPIHC]":
        cur_c = client.recv(1).decode()
        ch += cur_c
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

        chipeur = read_until_next_pipe_UTF16(client_socket)
        if chipeur != "[CHIPEUR]":
            print("pas chipeur")
            client_socket.close()

        username = read_until_next_pipe_UTF16(client_socket)
        print(username)

        datatype = read_until_next_pipe_UTF16(client_socket)
        print(datatype)

        if datatype == "file":
            filename = read_until_next_pipe_UTF16(client_socket)
            print(filename)
            savefile(username, datatype, filename, client_socket)
        client_socket.close()



if __name__ == "__main__":
    start_server()
