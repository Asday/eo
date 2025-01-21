CXX = gcc
CXXFLAGS = \
	-std=c++23 \
	-O3 \
	-Wall \
	-Wextra \
	-Werror \
	-Wsign-conversion \
	-pedantic-errors \
	-g

.PHONY: all
all: eo-server eo-client

eo-server:
	$(CXX) $(CXXFLAGS) $(LDFLAGS) server/main.cpp -o $@

eo-client:
	$(CXX) $(CXXFLAGS) $(LDFLAGS) client/main.cpp -o $@

.PHONY: clean
clean:
	rm eo-server eo-client
