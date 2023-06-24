#pragma once
#include <string>
#include <map>

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
private:
    static const std::map<char, Type> type_chars;
};

const std::map<char, Request::Type> Request::type_chars = {
    {'M',Music},
    {'V',Video},
    {'P',Photo},
};