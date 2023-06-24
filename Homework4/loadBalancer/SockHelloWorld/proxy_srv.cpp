#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <csignal>
#include <map>
#include <optional>
#include <vector>

using namespace std;

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
        static optional<int> cnt;
        if(!cnt.has_value()){
            cnt.emplace(1);
        }
        return cnt.value()++;
    }
    static SocketManager* gman;
    map<int, int> open_sockets;
};

SocketManager* SocketManager::gman = nullptr;

class Fifo{
public:
    void push(int item){
        queue.push_back(item);
    }
    int pop(){
        if(queue.size() == 0){
            throw std::runtime_error("requested to pop nonexistent item");
        }
        int res = queue.front();
        queue.erase(queue.begin());
        return res;
    }
private:
    std::vector<int> queue;
};

static int echo_srv_socknum;//TODO: maybe need to seperate read from write?

// Also signal handler function
//TODO: set signahandler to this:
void releaseResources(int signum) {
    SocketManager::deallocate();
    exit(signum);
}

void init_echo_srv_session() {
    // Create the socket
    echo_srv_socknum = socket(AF_INET, SOCK_STREAM, 0);
    if (echo_srv_socknum == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        releaseResources(1);
    }

    // Set up the server address
    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(80);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address

    // Connect to the server
    if (connect(echo_srv_socknum, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to connect to server" << std::endl;
        releaseResources(1);
    }
    SocketManager::get().insertSocket(echo_srv_socknum);
}

void send_req_to_echo_server(const string& msg) {
    // Send the message to the server
    if (send(echo_srv_socknum, msg.c_str(), msg.size(), 0) == -1) {
        std::cerr << "Failed to send message to server" << std::endl;;
        releaseResources(1);
    }
}

bool check_resp_from_echo_server(string& resp) {
    char buffer[1024];
    int bytesRead = recv(echo_srv_socknum, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);

    if (bytesRead == -1) {
        // No data available
        return false;
    } else if (bytesRead == 0) {
        printf("Echo Server has disconnected\n");
        releaseResources(1);
    }
    resp = std::string(buffer, bytesRead);
    printf("[Info] Response from echo-server: '%s'\n", resp.c_str());
    return true;
}

int main(){
    // Start connection with echo server:
    init_echo_srv_session();

    auto sm = SocketManager::get();

    // Create a socket
    int req_listen_socknum = socket(AF_INET, SOCK_STREAM, 0);//TCP
    if (req_listen_socknum == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        releaseResources(1);
    }
    sm.insertSocket(req_listen_socknum);

    // Prepare the server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8888);

    // Bind the socket to the specified IP address and port
    if (bind(req_listen_socknum, reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind socket." << std::endl;
        releaseResources(1);
    }

    // Set socket to non-blocking mode
    int flags = fcntl(req_listen_socknum, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Failed to get socket flags." << std::endl;
        releaseResources(1);
    }
    if (fcntl(req_listen_socknum, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set socket to non-blocking mode." << std::endl;
        releaseResources(1);
    }   

    Fifo server_fifo;

    while(true){
        // Listen for incoming connections
        if (listen(req_listen_socknum, 100) < 0) {
            std::cerr << "Failed to listen." << std::endl;
            releaseResources(1);
        }

        // Accept a client connection
        int client_req_socknum = accept(req_listen_socknum, nullptr, nullptr);
        int client_session_key = sm.insertSocket(client_req_socknum);

        if (client_req_socknum >= 0) {
            std::cout << "Client connected." << std::endl;
            
            // Get message from client and print it:
            char buffer[1024];

            int bytesRead = recv(client_req_socknum, buffer, sizeof(buffer)-1, 0);//blocking?
            std::string s(buffer, bytesRead);
            if (bytesRead == -1) {
                std::cerr << "Failed to receive message from client" << std::endl;
            } else if (bytesRead == 0) {
                std::cout << "Client disconnected" << std::endl;
            } else {
                printf("[Info] Recived: '%s'\n", s.c_str());
                s = "@"+s+"@";
                server_fifo.push(client_session_key);
                send_req_to_echo_server(s);
            }
        } else {
            //Check if got new response from the echo server:
            string resp;
            if(check_resp_from_echo_server(resp)){
                int client_session_key = server_fifo.pop();
                int client_session_socknum = sm.getSocknum(client_session_key);
                send(client_session_socknum, resp.c_str(), resp.size(), 0);
                sm.closeSocket(client_session_key);
                //TODO: check which client should actually get this response (have a list of open connections...)
            }
        }
        // printf(".\n");
    }
    //the only way out is interrupt or error which releases resources.
    return 0;
}







// string get_from_echo_server(const string& msg){
//     // Create the socket
//     int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (clientSocket == -1) {
//         std::cerr << "Failed to create socket" << std::endl;
//         exit(1);
//     }

//     // Set up the server address
//     struct sockaddr_in serverAddress{};
//     serverAddress.sin_family = AF_INET;
//     serverAddress.sin_port = htons(80);
//     serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address

//     // Connect to the server
//     if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
//         std::cerr << "Failed to connect to server" << std::endl;
//         close(clientSocket);
//         exit(1);
//     }

//     // Send the message to the server
//     if (send(clientSocket, msg.c_str(), msg.size(), 0) == -1) {
//         std::cerr << "Failed to send message to server" << std::endl;
//         close(clientSocket);
//         exit(1);
//     }

//     // Receive and print the server's response
//     char buffer[1024];
//     int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
//     std::string s(buffer, bytesRead);
//     if (bytesRead == -1) {
//         std::cerr << "Failed to receive response from server" << std::endl;
//     } else if (bytesRead == 0) {
//         std::cout << "Server disconnected" << std::endl;
//     } else {
//         printf("[Info] Response from echo-server: '%s'\n", s.c_str());
//     }
//     // Close the socket
//     close(clientSocket);

//     return s;
// }