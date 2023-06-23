#pragma once
#include "LBCore.hpp"

class RoundRobinLBCore : public LBCore{
public:
    LBCore()
        idx(0){}
    virtual ~LBCore(){}

    ServerDescriptor handleRequest(const RequestDetails& req){
        idx = (idx+1)%NUM_SERVERS;
        return idx;
    }
private:
    int idx;
};