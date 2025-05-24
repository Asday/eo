-include build/config.make
-include build/config-server.make

.SUFFIXES:
MAKEFLAGS += --no-builtin-rules

.PHONY: all
all: cluster launcher login client

build/artefacts:
	mkdir build/artefacts

build/artefacts/shared: build/artefacts
	mkdir build/artefacts/shared

cluster: src/servers/cluster/main.cpp
cluster: build/config.make
cluster: build/config-server.make
	$(CXX) \
		$(CXXFLAGS-server) $(CXXFLAGS) \
		$(LDFLAGS-server) $(LDFLAGS) \
		src/servers/cluster/main.cpp \
		-o $@

launcher: src/servers/launcher/main.cpp
launcher: build/config.make
launcher: build/config-server.make
	$(CXX) \
		$(CXXFLAGS-server) $(CXXFLAGS) \
		$(LDFLAGS-server) $(LDFLAGS) \
		src/servers/launcher/main.cpp \
		-o $@

login: src/servers/login/main.cpp
login: build/config.make
login: build/config-server.make
	$(CXX) \
		$(CXXFLAGS-server) $(CXXFLAGS) \
		$(LDFLAGS-server) $(LDFLAGS) \
		src/servers/login/main.cpp \
		-o $@

client: src/client/main.cpp build/config.make
	$(CXX) $(CXXFLAGS) $(LDFLAGS) src/client/main.cpp -o $@

.PHONY: clean
clean:
	rm -f cluster launcher login client
	rm -rf build/artefacts
