#include <vector>
#include <map>
#include <functional>
#include "../Common/Request.hpp"

typedef std::function<void()> Event;

Event createClientRequestEvent()

int main(int argc, char** argv){
    static constexpr unsigned NUM_CLIENTS = 5;
    LBCore& lb = *dispatchLBCore(argc, argv);
    
    std::vector<std::vector<Request>> client_requests_lists;
    for(unsigned client_idx = NUM_CLIENTS-1; client_idx >= 0; --client_idx){
        std::string client_requests_filename = "h"+std::to_string(client_idx)+".in";
        client_requests_lists.push_back(parseRequestsFile(client_requests_filename));
    }

    std::map<unsigned, Event> event_map;
    for(std::vector<Request>& req_list: client_requests_lists){
        if(req_list.size() > 0){
            Request& first_request = req_list[0];
            // event_map.emplace();
        }
    }
}