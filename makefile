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
CXXFLAGS-server = -I$(shell pg_config --includedir)
LDFLAGS-server = -L$(shell pg_config --libdir) -lpq

.PHONY: all
all: eo-server eo-client

eo-server: server/main.cpp makefile
	$(CXX) \
		$(CXXFLAGS) $(CXXFLAGS-server) \
		$(LDFLAGS-server) $(LDFLAGS) \
		server/main.cpp \
		-o $@

eo-client: client/main.cpp makefile
	$(CXX) $(CXXFLAGS) $(LDFLAGS) client/main.cpp -o $@

.PHONY: clean
clean:
	rm eo-server eo-client
