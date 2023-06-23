#pragma once

typedef int ServerDescriptor;

struct RequestDetails{
    enum RequestType{

    };
    RequestType type;
    size_t handle_time;
};


class LBCore{
public:
    static constexpr size_t NUM_SERVERS = 3;

    LBCore();
    virtual ~LBCore() = 0;

    ServerDescriptor handleRequest(const RequestDetails& req) = 0;
};