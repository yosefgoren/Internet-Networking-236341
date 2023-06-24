import socket

LOAD_BALANCER_HOST = '0.0.0.0'
LOAD_BALANCER_PORT = 8888

SERVER_HOST = '192.168.0.101'
SERVER_PORT = 80

def main():
    # Create a load balancer socket
    load_balancer_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    load_balancer_socket.bind((LOAD_BALANCER_HOST, LOAD_BALANCER_PORT))
    load_balancer_socket.listen(1)
    print('Load balancer started on {}:{}'.format(LOAD_BALANCER_HOST, LOAD_BALANCER_PORT))

    # Connect to the server
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.connect((SERVER_HOST, SERVER_PORT))
    print('Connected to server {}:{}'.format(SERVER_HOST, SERVER_PORT))

    while True:
        # Accept client connections
        client_socket, client_address = load_balancer_socket.accept()
        print('Accepted client connection from {}:{}'.format(client_address[0], client_address[1]))

        # Forward client request to the server
        request = client_socket.recv(1024)
        print('Received request from client:', request.decode())

        # Send client request to the server
        server_socket.sendall(request)

        # Receive server response
        response = server_socket.recv(1024)
        print('Received response from server:', response.decode())

        # Send server response back to the client
        client_socket.sendall(response)

        # Close the client socket
        client_socket.close()

    # Close the server socket
    server_socket.close()

if __name__ == '__main__':
    main()

