#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

using namespace std;

string get_from_echo_server(const string& msg){
    // Create the socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        exit(1);
    }

    // Set up the server address
    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(80);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to connect to server" << std::endl;
        close(clientSocket);
        exit(1);
    }

    // Send the message to the server
    if (send(clientSocket, msg.c_str(), msg.size(), 0) == -1) {
        std::cerr << "Failed to send message to server" << std::endl;
        close(clientSocket);
        exit(1);
    }

    // Receive and print the server's response
    char buffer[1024];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    std::string s(buffer, bytesRead);
    if (bytesRead == -1) {
        std::cerr << "Failed to receive response from server" << std::endl;
    } else if (bytesRead == 0) {
        std::cout << "Server disconnected" << std::endl;
    } else {
        printf("[Info] Response from echo-server: '%s'\n", s.c_str());
    }
    // Close the socket
    close(clientSocket);

    return s;
}

int main(){
    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);//TCP
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    // Prepare the server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8888);

    // Bind the socket to the specified IP address and port
    if (bind(serverSocket, reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind socket." << std::endl;
        return 1;
    }

    while(true){
        // Listen for incoming connections
        if (listen(serverSocket, 3) < 0) {
            std::cerr << "Failed to listen." << std::endl;
            return 1;
        }

        std::cout << "Server listening on port 8888..." << std::endl;

        // Accept a client connection
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            std::cerr << "Failed to accept connection." << std::endl;
            return 1;
        }

        std::cout << "Client connected." << std::endl;
        
        // Get message from client and print it:
        char buffer[1024];

        int bytesRead = recv(clientSocket, buffer, sizeof(buffer)-1, 0);//blocking?
        std::string s(buffer, bytesRead);
        if (bytesRead == -1) {
            std::cerr << "Failed to receive message from client" << std::endl;
        } else if (bytesRead == 0) {
            std::cout << "Client disconnected" << std::endl;
        } else {
            printf("[Info] Recived: '%s'\n", s.c_str());
            s = "@"+s+"@";
            string res = get_from_echo_server(s);
            send(clientSocket, res.c_str(), res.size(), 0);
        }
        
        // Close the sockets
        close(clientSocket);
    }

    close(serverSocket);

    return 0;
}



