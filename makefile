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

build/artefacts/shared/db.o: src/shared/db.cpp
build/artefacts/shared/db.o: src/shared/db.h
build/artefacts/shared/db.o: build/config.make
build/artefacts/shared/db.o: build/artefacts/shared
	$(CXX) \
		$(CXXFLAGS) \
		src/shared/db.cpp \
		-c \
		-o $@

build/artefacts/shared/lg.o: src/shared/lg.cpp
build/artefacts/shared/lg.o: src/shared/lg.h
build/artefacts/shared/lg.o: build/config.make
build/artefacts/shared/lg.o: build/artefacts/shared
	$(CXX) \
		$(CXXFLAGS) \
		src/shared/lg.cpp \
		-c \
		-o $@

build/artefacts/shared/repo.o: src/shared/repo.cpp
build/artefacts/shared/repo.o: src/shared/repo.h
build/artefacts/shared/repo.o: src/shared/db.h
build/artefacts/shared/repo.o: src/shared/lg.h
build/artefacts/shared/repo.o: build/config.make
build/artefacts/shared/repo.o: build/artefacts/shared
	$(CXX) \
		$(CXXFLAGS) \
		src/shared/repo.cpp \
		-c \
		-o $@

build/artefacts/shared/signal.o: src/shared/signal.cpp
build/artefacts/shared/signal.o: src/shared/signal.h
build/artefacts/shared/signal.o: build/config.make
build/artefacts/shared/signal.o: build/artefacts/shared
	$(CXX) \
		$(CXXFLAGS) \
		src/shared/signal.cpp \
		-c \
		-o $@

cluster: src/servers/cluster/main.cpp
cluster: build/artefacts/shared/db.o
cluster: build/artefacts/shared/lg.o
cluster: build/artefacts/shared/repo.o
cluster: build/artefacts/shared/signal.o
cluster: build/config.make
cluster: build/config-server.make
	$(CXX) \
		$(CXXFLAGS-server) $(CXXFLAGS) \
		$(LDFLAGS-server) $(LDFLAGS) \
		src/servers/cluster/main.cpp \
		build/artefacts/shared/db.o \
		build/artefacts/shared/lg.o \
		build/artefacts/shared/repo.o \
		build/artefacts/shared/signal.o \
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
