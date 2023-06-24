#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <csignal>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "../LBCore/RoundRobinLBCore.hpp"
#include "../Common/Fifo.hpp"

class SocketManager{
public:
    static SocketManager& get(){
        if(gman == nullptr){
            gman = new SocketManager();
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
            for(auto& [socket_key, socket_num]: sm.open_sockets){
                close(socket_num);
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
    static int find_free_socket_key(){
        static std::optional<int> cnt;
        if(!cnt.has_value()){
            cnt.emplace(1);
        }
        return cnt.value()++;
    }
    static SocketManager* gman;
    std::map<int, int> open_sockets;
};

void releaseResources(int signum) {
    SocketManager::deallocate();
    exit(signum);
}

class WorkerServerManager{
public:
    WorkerServerManager(const std::string& ipaddr, int worker_listen_port){
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

        // Connect to the server
        if (connect(worker_server_socknum, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            throw std::runtime_error("Failed to connect to server");
        }
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
        printf("[Info] Response from worker server: '%s'\n", resp.c_str());
        
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

int main(int argc, char** argv){
    static const std::vector<std::string> WORKER_SERVER_IPS = {"192.168.0.101", "192.168.0.102", "192.168.0.103"};
    // static constexpr auto WORKER_NET_IFACE = 
    // static constexpr auto CLIENT_IFACE = "10.0.0.1";
    static constexpr int LISTEN_PORT = 80;

    signal(SIGTERM, releaseResources);    
    try{
        //TODO: implement dynamic LBCore subclass dispatch based on argv?

        //Choose load balancer algorithm implementation/policy:
        LBCore& lb = *std::shared_ptr<LBCore>(new RoundRobinLBCore());
        SocketManager& sm = SocketManager::get();

        //Open connections with servers:
        std::vector<WorkerServerManager> workers;
        for(const std::string& ipaddr : WORKER_SERVER_IPS){
            workers.push_back(WorkerServerManager(ipaddr, LISTEN_PORT));
        }

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

        while(true){
            // Listen for incoming connections
            if (listen(req_listen_socknum, 100) < 0) {
                throw std::runtime_error("Failed to listen");
            }

            // Accept a client connection
            int client_req_socknum = accept(req_listen_socknum, nullptr, nullptr);
            int client_session_key = sm.insertSocket(client_req_socknum);

            if(client_req_socknum >= 0) {
                printf("Client connected\n");
                
                // Get message from client and print it:
                char buffer[1024];

                int bytesRead = recv(client_req_socknum, buffer, sizeof(buffer)-1, 0);//blocking?
                std::string client_req(buffer, bytesRead);
                if (bytesRead == -1) {
                    throw std::runtime_error("Failed to receive message from client");
                } else if (bytesRead == 0) {
                    std::cout << "Client disconnected" << std::endl;
                } else {
                    printf("[Info] Recived: '%s'\n", client_req.c_str());
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
        printf("LBEngine stopped due to unexpected error: '%s'\n", e.what());
        releaseResources(1);
    }
    return 0;
}