#pragma once
#include "LBCore.hpp"

class RoundRobinLBCore : public LBCore{
public:
    RoundRobinLBCore()
        :idx(0){}
    virtual ~RoundRobinLBCore(){}

    virtual int handleRequest(const char* req) override{
        idx = (idx+1)%NUM_SERVERS;
        return idx;
    }

    virtual void notify(int server_idx) override{}
private:
    int idx;
};