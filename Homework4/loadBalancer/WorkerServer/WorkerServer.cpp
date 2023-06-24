#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

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
    serverAddress.sin_port = htons(80);

    // Bind the socket to the specified IP address and port
    if (bind(serverSocket, reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind socket." << std::endl;
        return 1;
    }

    if (listen(serverSocket, 3) < 0) {
        std::cerr << "Failed to listen." << std::endl;
        return 1;
    }

    std::cout << "Server listening on port 80..." << std::endl;

    // Accept a client connection
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket < 0) {
        std::cerr << "Failed to accept connection." << std::endl;
        return 1;
    }

    std::cout << "Proxy connected." << std::endl;
    
    while(true){
        // Listen for incoming connections
        // Get message from client and print it:
        char buffer[1024];

        int bytesRead = recv(clientSocket, buffer, sizeof(buffer)-1, 0);//blocking?
        std::string s(buffer, bytesRead);
        if (bytesRead == -1) {
            std::cerr << "Failed to receive message from client" << std::endl;
        } else if (bytesRead == 0) {
            std::cout << "Proxy disconnected" << std::endl;
            break;
        } else {
            printf("[Info] Recived: '%s'\n", s.c_str());
            sleep(2);
            s = "!"+s+"!";
            send(clientSocket, s.c_str(), s.size(), 0);
        }
        
    }
    // Close the sockets
    close(clientSocket);
    close(serverSocket);

    return 0;
}