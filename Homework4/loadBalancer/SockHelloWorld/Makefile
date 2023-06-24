CXX=clang++
FLAGS=-std=c++17

all: echo_srv.out proxy_srv.out

echo_srv.out: echo_srv.cpp
	$(CXX) $(FLAGS) $^ -o $@

proxy_srv.out: proxy_srv.cpp
	$(CXX) $(FLAGS) $^ -o $@

clean:
	rm -f echo_srv.out proxy_srv.out