#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <csignal>
#include <map>
#include <memory>
#include <vector>

#include "../LBCore/LBCore.hpp"
#include "../Common/Fifo.hpp"

class SocketManager{
public:
    static SocketManager& get(){
        if(gman == nullptr){
            gman = new SocketManager();
            gman->min_free_socket_key = 0;
        }
        return *gman;
    }

    /**
     * @returns a socket key that can be used to refer to this session later.
    */
    int insertSocket(int socket_num){
        int socket_key = find_free_socket_key();
        open_sockets.emplace(socket_key, socket_num);
        return socket_key;
    }

    static void deallocate(){
        if(gman != nullptr){
            auto sm = SocketManager::get();
            for(auto& socket_key_and_socket_num: sm.open_sockets){
                close(socket_key_and_socket_num.second);
            }
            delete gman;
            gman = nullptr;
        }
    }

    int getSocknum(int socket_key){
        return open_sockets.at(socket_key);
    }

    void closeSocket(int socket_key){
        int socket_num = open_sockets.at(socket_key);
        close(socket_num);
        open_sockets.erase(socket_key);
    }

private:
    /**
     * @return a key which is unused in 'open_sockets':
    **/
    int find_free_socket_key(){
        return min_free_socket_key++;
    }
    static SocketManager* gman;
    std::map<int, int> open_sockets;
    int min_free_socket_key;
};

void releaseResources(int signum) {
    printf("Deallocating sockets and exiting...\n");
    SocketManager::deallocate();
    exit(signum);
}

class WorkerServerManager{
public:
    WorkerServerManager(const std::string& ipaddr, int worker_listen_port){
        printf("Attempting to connect to worker server at: %s:%d\n", ipaddr.c_str(), worker_listen_port);
        // Create the socket
        worker_server_socknum = socket(AF_INET, SOCK_STREAM, 0);
        if (worker_server_socknum == -1) {
            throw std::runtime_error("Failed to create socket");
        }

        // Set up the server address
        struct sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(worker_listen_port);
        serverAddress.sin_addr.s_addr = inet_addr(ipaddr.c_str());

        // Set the timeout value (in milliseconds)
        int timeout_millisec = 2000; // Example timeout value of 5 seconds
        struct timeval timeout{};
        timeout.tv_sec = timeout_millisec / 1000;
        timeout.tv_usec = (timeout_millisec % 1000) * 1000;

        // Set the socket option for receive timeout
        if (setsockopt(worker_server_socknum, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
            throw std::runtime_error("Failed to set receive timeout");
        }
        // Set the socket option for send timeout
        if (setsockopt(worker_server_socknum, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
            throw std::runtime_error("Failed to set send timeout");
        }

        // Connect to the server
        if (connect(worker_server_socknum, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            // Check if the error indicates a timeout
            if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
                throw std::runtime_error("Could not connect to worker server due to timeout");
            } else {
                throw std::runtime_error("Failed to connect to server");
            }
        }
        printf("Succesfully connected to worker server\n");
        SocketManager::get().insertSocket(worker_server_socknum);
    }
    WorkerServerManager() = delete;
    ~WorkerServerManager() = default;
    
    void sendRequest(const std::string& msg, int client_session_key) {
        // Send the message to the server
        if (send(worker_server_socknum, msg.c_str(), msg.size(), 0) == -1) {
            throw std::runtime_error("Failed to send message to worker server");
        }
        request_fifo.push(client_session_key);
    }

    /**
     * @returns whether a response was recived and handeled.
    */
    bool handleResponse() {
        char buffer[1024];
        int bytesRead = recv(worker_server_socknum, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);

        if (bytesRead == -1) {
            return false;
        } else if (bytesRead == 0) {
            throw std::runtime_error("worker server has disconnected");
        }
        std::string resp = std::string(buffer, bytesRead);
        printf("Response from worker server: '%s'\n", resp.c_str());
        
        int client_session_key = request_fifo.pop();
        int client_session_socknum = SocketManager::get().getSocknum(client_session_key);
        send(client_session_socknum, resp.c_str(), resp.size(), 0);
        SocketManager::get().closeSocket(client_session_key);
        return true;
    }

private:
    int worker_server_socknum;
    Fifo request_fifo;
};

SocketManager* SocketManager::gman = nullptr;

std::string getPeerAddress(int socknum) {
    struct sockaddr_in address{};
    socklen_t addr_length = sizeof(socknum);

    // Get the client address information
    if (getpeername(socknum, (struct sockaddr*)&address, &addr_length) == -1) {
        throw std::runtime_error("Failed to get peer address");
    }

    char peerIP[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(address.sin_addr), peerIP, INET_ADDRSTRLEN) == nullptr) {
        throw std::runtime_error("Failed to convert peer IP");
    }

    return std::string(peerIP);
}

int main(int argc, char** argv){
    static const std::vector<std::string> WORKER_SERVER_IPS = {"192.168.0.101", "192.168.0.102", "192.168.0.103"};
    static const int LISTEN_PORT = 80;

    signal(SIGTERM, releaseResources);   
    try{
        //Choose load balancer algorithm implementation/policy:
        LBCore& lb = *dispatchLBCore(argc, argv);
        
        //The socket manager remembers all of the open sessions. It is a singleton.
        SocketManager& sm = SocketManager::get();

        //Open connections with servers:
        std::vector<WorkerServerManager> workers;
        for(const std::string& ipaddr : WORKER_SERVER_IPS){
            workers.push_back(WorkerServerManager(ipaddr, LISTEN_PORT));
        }
        printf("finished connecting to all worker servers\n");

        //Open socket for listening to clients:
        int req_listen_socknum = socket(AF_INET, SOCK_STREAM, 0);//TCP
        if (req_listen_socknum == -1) {
            throw std::runtime_error("Failed to create request listen socket");
        }
        sm.insertSocket(req_listen_socknum);

        // Prepare the server address
        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        serverAddress.sin_port = htons(LISTEN_PORT);

        // Bind the socket to the specified IP address and port
        if (bind(req_listen_socknum, reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress)) < 0) {
            throw std::runtime_error("Failed to bind socket");
        }

        // Set socket to non-blocking mode
        int flags = fcntl(req_listen_socknum, F_GETFL, 0);
        if (flags == -1) {
            throw std::runtime_error("Failed to get socket flags");
        }
        if (fcntl(req_listen_socknum, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw std::runtime_error("Failed to set socket to non-blocking mode");
        }   

        printf("Finished creating request listenting socket");

        while(true){
            // Listen for incoming connections
            if (listen(req_listen_socknum, 100) < 0) {
                throw std::runtime_error("Failed to listen");
            }

            // Accept a client connection
            int client_req_socknum = accept(req_listen_socknum, nullptr, nullptr);
            int client_session_key = sm.insertSocket(client_req_socknum);

            if(client_req_socknum >= 0) {
                printf("New connection to client at: '%s'\n", getPeerAddress(client_req_socknum).c_str());
                
                // Get message from client and print it:
                char buffer[1024];

                int bytesRead = recv(client_req_socknum, buffer, sizeof(buffer)-1, 0);//blocking?
                std::string client_req(buffer, bytesRead);
                if (bytesRead == -1) {
                    throw std::runtime_error("Failed to receive message from client");
                } else if (bytesRead == 0) {
                    std::cout << "Client disconnected" << std::endl;
                } else {
                    printf("Recived '%s' from client.\n", client_req.c_str());
                    int handling_server_idx = lb.handleRequest(client_req.c_str());
                    workers[handling_server_idx].sendRequest(client_req, client_session_key);
                }
            }
            //Forward any responses sent by the worker servers:
            for(int worker_server_idx = 0; worker_server_idx < workers.size(); ++worker_server_idx) {
                if(workers.at(worker_server_idx).handleResponse()){
                    lb.notify(worker_server_idx);
                }
            }
        }
    } catch(std::runtime_error& e){
        printf("LBEngine error: '%s'\n", e.what());
    } catch(...){
        printf("LBEngine encountered an unexpected exception\n");
    }
    releaseResources(1);
    return 0;
}