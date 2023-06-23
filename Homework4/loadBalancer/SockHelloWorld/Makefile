CXX=clang++

all: echo_srv.out proxy_srv.out

echo_srv.out: echo_srv.cpp
	$(CXX) $^ -o $@

proxy_srv.out: proxy_srv.cpp
	$(CXX) $^ -o $@

clean:
	rm -f echo_srv.out proxy_srv.out