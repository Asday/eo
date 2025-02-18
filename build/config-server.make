CXXFLAGS-server = -I$(shell pg_config --includedir)
LDFLAGS-server = -L$(shell pg_config --libdir) -lpq
