#pragma once
#include <algorithm>
#include "../Common/Request.hpp"
#include "../Common/WorkerServerInfo.hpp"

class LBCore{
public:
    static const unsigned NUM_WORKER_SERVERS = 3;

    virtual ~LBCore(){}
    virtual int handleRequest(const char* req) = 0;
    /**
     * notify the LBCore algorithm that server at idx @param server_idx has finished
     * handling it's 'front' request.
    */
    virtual void notify(unsigned server_idx) = 0;
};

class GreedyLBCore : public LBCore{
    /**
     * The greedy dispatch algorithm optimizes the final request finishing time
     * while at any point where a new request is issued to it - it assumes it is the last request.
    */
public:
    virtual ~GreedyLBCore() override = default;
    GreedyLBCore(){
        times_to_finish = std::vector<unsigned>(0, NUM_WORKER_SERVERS);
        worker_infos = get_server_infos();
    }

    virtual int handleRequest(const char* req_str) override{
        Request req(req_str);

        unsigned min_finish_time = 0xffffffff;//max int
        unsigned min_server_idx = 0;
        //For each server - estimate time to finish if request dispatch to this server,
        //  choose the server that will finish first assuming no more reqeusts will be dispatched.
        for(unsigned server_idx = 0; server_idx < NUM_WORKER_SERVERS; ++server_idx){
            unsigned duration_modifer = worker_infos[server_idx]->getDurationModifer(req.type);
            unsigned new_finish_time = times_to_finish[server_idx]+req.duration_secs*duration_modifer;
            if(new_finish_time < min_finish_time){
                min_finish_time = new_finish_time;
                min_server_idx = server_idx;
            }
        }
        //Update the estimated finish time of the chosen server:
        times_to_finish[min_server_idx] = min_finish_time;
        return min_server_idx;
    }

    /**
     * notify the LBCore algorithm that server at idx @param server_idx has finished
     * handling it's 'front' request.
    */
    virtual void notify(unsigned server_idx) override{
        unsigned time_passed = times_to_finish[server_idx];
        for(unsigned& time: times_to_finish){
            time = std::max(0u, time-time_passed);
        }
    }
private:
    std::vector<std::shared_ptr<WorkerServerInfo>> worker_infos;
    std::vector<unsigned> times_to_finish;
};


class RoundRobinLBCore : public LBCore{
public:
    virtual ~RoundRobinLBCore() override = default;
    RoundRobinLBCore()
        :idx(0){}

    virtual int handleRequest(const char* req) override{
        idx = (idx+1)%NUM_WORKER_SERVERS;
        return idx;
    }

    virtual void notify(unsigned server_idx) override{}
private:
    int idx;
};


std::shared_ptr<LBCore> dispatchLBCore(const std::string& desciption){
    if(desciption == "rr")
        return std::shared_ptr<LBCore>(new RoundRobinLBCore());
    else if(desciption == "greedy")
        return std::shared_ptr<LBCore>(new GreedyLBCore());
    else
        throw std::runtime_error("LBCore dispatch function got unexpected description");
}

std::shared_ptr<LBCore> getDefaultLBCore(){
    return dispatchLBCore("greedy");
}

std::shared_ptr<LBCore> dispatchLBCore(int argc, char** argv){
    return argc >= 2 ? dispatchLBCore(argv[1]) : getDefaultLBCore();
}