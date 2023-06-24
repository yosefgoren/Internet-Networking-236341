#pragma once

class LBCore{
public:
    static constexpr size_t NUM_SERVERS = 3;

    virtual int handleRequest(const char* req) = 0;
    /**
     * notify the LBCore algorithm that server at idx @param server_idx has finished
     * handling it's 'front' request.
    */
    virtual void notify(int server_idx) = 0;
};