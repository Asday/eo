CXX = g++
CXXFLAGS = \
	-std=c++23 \
	-O3 \
	-Wall \
	-Wextra \
	-Werror \
	-Wsign-conversion \
	-pedantic-errors \
	-g
LDFLAGS =

.PHONY: all
all: eo-server eo-client

eo-server: server/main.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) server/main.cpp -o $@

eo-client: client/main.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) client/main.cpp -o $@

.PHONY: clean
clean:
	rm eo-server eo-client
