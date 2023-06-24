CXX=g++
FLAGS=-Wall -g -Werror -std=c++11 -Wno-sign-compare

load_balancer.out: LBEngine/LBEngine.cpp LBCore/LBCore.hpp Common/Fifo.hpp
	$(CXX) $(FLAGS) LBEngine/LBEngine.cpp -o $@

clean:
	rm -f load_balancer.out