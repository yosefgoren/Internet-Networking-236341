CXX=clang++
FLAGS=-Wall -Werror -std=c++17 -ferror-limit=1

load_balancer.out: LBEngine/LBEngine.cpp LBCore/LBCore.hpp Common/Fifo.hpp
	$(CXX) $(FLAGS) LBEngine/LBEngine.cpp -o $@

clean:
	rm -f load_balancer.out