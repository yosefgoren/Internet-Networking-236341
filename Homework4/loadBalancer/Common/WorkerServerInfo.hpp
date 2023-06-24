#pragma once
#include <vector>
#include <memory>
#include "Request.hpp"

class WorkerServerInfo{
public:
    virtual ~WorkerServerInfo(){};
    virtual unsigned getDurationModifer(Request::Type type) = 0;

    virtual unsigned getCompletionTime(const Request& req){
        return req.duration_secs*getDurationModifer(req.type);
    }
};

class VideoServerInfo : public WorkerServerInfo{
public:
    virtual ~VideoServerInfo() override = default;
    virtual unsigned getDurationModifer(Request::Type type) override {
        switch(type){
            case Request::Type::Music: return 2;
            case Request::Type::Video: return 1;
            case Request::Type::Photo: return 1;
        }
    }
};

class MusicServerInfo : public WorkerServerInfo{
public:
    virtual ~MusicServerInfo() override = default;
    virtual unsigned getDurationModifer(Request::Type type) override {
        switch(type){
            case Request::Type::Music: return 1;
            case Request::Type::Video: return 3;
            case Request::Type::Photo: return 2;
        }
    }
};

std::vector<std::shared_ptr<WorkerServerInfo>> get_server_infos(){
    return {
        std::shared_ptr<WorkerServerInfo>(new VideoServerInfo()),
        std::shared_ptr<WorkerServerInfo>(new VideoServerInfo()),
        std::shared_ptr<WorkerServerInfo>(new MusicServerInfo())
    };
}