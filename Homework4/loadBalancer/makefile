COMMON_FLAGS=-Wall -g -Werror -Wno-sign-compare
GPP_FLAGS=$(COMMON_FLAGS) -std=c++11
CLANG_FLAGS=$(COMMON_FLAGS) -std=c++17 -ferror-limit=1
GPP=g++ $(GPP_FLAGS)
CLANG=clang++ $(CLANG_FLAGS)

load_balancer: LBEngine/LBEngine.cpp LBCore/LBCore.hpp Common/Fifo.hpp
	$(GPP) LBEngine/LBEngine.cpp -o $@

clean:
	rm -f load_balancer emulator