#include <iostream>
#include <string>
#include <memory>
#include <array>
#include <stdint.h>
#include <vector>

#include "../LBCore/RoundRobinLBCore.hpp"

struct ClientDescriptor{
    uint32_t ip;
    uint32_t client_port;
};

typedef array<uint8_t> Buf;

struct Request : public RequestDetails{
    Request(const ClientDescriptor& cdesc, RequestType type, size_t handle_time)
        :RequestDetails(type, handle_time), cdesc(cdesc){}
    ClientDescriptor cdesc;
};

struct ClientConnection{
    //...
};


static constexpr auto CLIENT_IFACE = "10.0.0.1";
static constexpr auto SERVER_IFACE = "192.168.0.1";

std::vector<ClientConnection> open_client_connections;

void forwardResponseToClient(const Buf& response_packet){
    //...
    //uses 'open_client_connections'...
}

int main(int argc, char** argv){
    //TODO: implement dynamic LBCore subclass dispatch based on argv?

    //Choose load balancer algorithm implementation/policy:
    LBCore& lb = *std::shared_ptr<LBCore>(new RoundRobinLBCore());

    //Open connections with servers:
    //...

    //Open socket for listening to clients:
    //...


    while(true){
        //the operation can either be a client request or a server response:
        bool is_client_request;
        Buf packet;
        //socket socket

        //Wait for either request from client or response from server:
        while(true){
            //Sniff for a new connection from a client - break loop if one is found:
            //...
            
            //Sniff for response from a server - break loop if one is found:
            //...
        }
        
        if(is_client_request){
            //Create request from the packet:
            Request req;//...
            
            ServerDescriptor desc = lb.handleRequest(req);

            //Dispatch the request to the server decided by the LB alg:
            //...
        } else {
            forwardResponseToClient(packet);
        }
    }
}

//============================================================================










#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int serverSocket, newSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientLength;
    char buffer[1024] = {0};
    std::string response = "Hello from the server!\n";

    // Create a socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Prepare the server address
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(80);

    // Bind the socket to the specified IP address and port
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        return 1;
    }

    // Start listening for incoming connections
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Failed to listen for connections" << std::endl;
        return 1;
    }

    std::cout << "Listening on 10.0.0.1:80..." << std::endl;

    while (true) {
        // Accept an incoming connection
        clientLength = sizeof(clientAddress);
        newSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientLength);
        if (newSocket < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            return 1;
        }

        // Read data from the client
        ssize_t bytesRead = read(newSocket, buffer, sizeof(buffer));
        if (bytesRead < 0) {
            std::cerr << "Failed to read from socket" << std::endl;
            return 1;
        }

        std::cout << "Received: " << buffer << std::endl;

        // Send response to the client
        ssize_t bytesSent = send(newSocket, response.c_str(), response.length(), 0);
        if (bytesSent < 0) {
            std::cerr << "Failed to send response" << std::endl;
            return 1;
        }

        // Close the connection
        close(newSocket);
    }

    close(serverSocket);
    return 0;
}