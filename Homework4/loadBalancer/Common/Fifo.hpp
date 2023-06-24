#pragma once

class Fifo{
public:
    void push(int item){
        queue.push_back(item);
    }
    int pop(){
        if(queue.size() == 0){
            throw std::runtime_error("requested to pop nonexistent item");
        }
        int res = queue.front();
        queue.erase(queue.begin());
        return res;
    }
private:
    std::vector<int> queue;
};