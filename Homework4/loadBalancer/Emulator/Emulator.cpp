#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "../Common/Request.hpp"
#include "../LBCore/LBCore.hpp"

class Emulator{
public:
    static constexpr unsigned NUM_CLIENTS = 5;
    typedef std::function<void()> Event;
    
    Emulator(int argc, char** argv)
            :lb(dispatchLBCore(argc, argv)){
        worker_infos = get_server_infos();
        for(unsigned client_idx = NUM_CLIENTS; client_idx > 0; client_idx--){
            std::string client_requests_filename = "h"+std::to_string(client_idx)+".in";
            printf("parsing requests file named '%s'.\n", client_requests_filename.c_str());
            client_requests_lists.push_back(Request::parseRequestsFile(client_requests_filename));
        }
        workers_times_to_finish = {0, 0, 0};

        for(unsigned client_idx = 0; client_idx < NUM_CLIENTS; ++client_idx){
            event_map.emplace(0, createClientRequestEvent(client_idx));
        }
    }

    unsigned run(){
        while(!event_map.empty()){
            Event& event = event_map.begin()->second;
            event();
        }
        int m = 0;
        for(int time: worker_total_times){
            m = m > time ? m : time;
        }
        return m;
    }
    
    Event createClientRequestEvent(int client_idx){
        return [=](){
            auto& client_requests_list = client_requests_lists[client_idx];
            Request next_client_request = client_requests_list.back();
            client_requests_list.pop_back();
            int server_idx = lb->handleRequest(next_client_request.to_string().c_str());
            int elapsed_time = worker_infos[server_idx]->getCompletionTime(next_client_request);
            workers_times_to_finish[server_idx] += elapsed_time;
            event_map.emplace(workers_times_to_finish[server_idx], createServerResponseEvent(client_idx, server_idx, elapsed_time));
        };
    }

    Event createServerResponseEvent(int server_idx, int client_idx, unsigned time_step){
        return [=](){
            workers_times_to_finish[server_idx] -= time_step;
            worker_total_times[server_idx] += time_step;
            lb->notify(server_idx);
            event_map.emplace(0, createClientRequestEvent(client_idx));
        };
    }

    std::map<unsigned, Event> event_map;

    std::shared_ptr<LBCore> lb;
    std::vector<std::shared_ptr<WorkerServerInfo>> worker_infos;
    std::vector<std::vector<Request>> client_requests_lists;
    std::vector<unsigned> workers_times_to_finish;
    std::vector<unsigned> worker_total_times;
};

int main(int argc, char** argv){
    Emulator e(argc, argv);
    printf("total time is: %d\n", e.run());
}