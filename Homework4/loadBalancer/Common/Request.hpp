#pragma once
#include <string>
#include <map>
#include <vector>
#include <fstream>

struct Request{
    Request(const std::string& src){
        if(src.size() != 2){
            throw std::runtime_error("Request c'tor expected source string to be exactly 2 characters");
        }
        type = type_chars.at(src[0]);
        duration_secs = src[1]-'0';
    }
    enum Type{
        Music,
        Video,
        Photo
    };

    Type type;
    size_t duration_secs;

    static std::vector<Request> parseRequestsFile(const std::string& filename){
        std::vector<Request> res;
        std::ifstream file(filename);
        if(!file.is_open()){
            throw std::runtime_error("could not open requests file");
        }
        std::string line;
        while (std::getline(file, line)) {
            if((line.size()&1) != 0){
                throw std::runtime_error("requests file bad format: expected even number of characters");
            }
            unsigned line_requests = line.size()/2;
            for(unsigned i = 0; i < line_requests; ++i){
                std::string src = line.substr(i*2, 2);
                res.push_back(Request(src));
            }
        }
        file.close();
        return res;
    }
private:
    static const std::map<char, Type> type_chars;
};

const std::map<char, Request::Type> Request::type_chars = {
    {'M',Music},
    {'V',Video},
    {'P',Photo},
};