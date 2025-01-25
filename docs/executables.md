# Cluster description

A cluster is made up of one or more physical machines and a PostgreSQL database.  Each machine launches a launcher executable on boot.  If there is more than one physical machine, one machine may be specially for the cluster executable and not run a launcher executable.  Each machine running a launcher has two network interfaces, public and private.  The public interface is exposed to the internet and is connected to by game clients.  The private interface is used for communication between servers, and is not available to the internet.

# Executables

## Internally accessible

### Launcher

Each launcher executable binds a UDP socket on whatever port it likes, and registers itself with the database in the `launcher` table, recording its internal IP and chosen port, along with a current timestamp, and compute/memory utilisation stats.  Periodically (each second, perhaps), these stats and timestamps are updated.

### Cluster

The cluster executable (which could be better named) is given a name to start with.  It connects to the database and polls it to find some suitable launcher instances, sends them messages over UDP to start a login server (with its own name as an argument), a chat server, and persistent services.  It polls to see those in the database, then presents a CLI.

The CLI has the following commands:

`shutdown <seconds>` - broadcast to connected game clients that a shutdown is imminent, then shut down after `seconds` seconds.

### Persistent services

Stuff that runs on timers that should be able to time out without an active grid:

* skill training
* reinforce timers
* cosmic signature/anomaly spawning
* industry jobs
* space junk cleanup

## Externally accessible

All externally accessible servers that own sessions maintain a heartbeat with each game client related to their owned sessions.  If a heartbeat lapses, the game client is assumed gone, its disconnection is logged, and the session is logged out.

### Login server

The login server is given a name to start with.  This name controls which data is seen by the server, and thus all game clients.  For example, "production" and "staging".

The login server advertises its internal IP in the database, then binds a UDP socket on the public interface to accept login attempts from game clients.

When a game client proffers a username and password, if they're correct and active, the login server collects character details from the database and sends them to the client.  It also generates a token and sends it to the client to use in all subsequent messages.  The token is stored in a new client session.

When a client sends a character selection message with the correct token, the character's information is loaded into the client session.  If a matching grid or station server (as applicable) is not advertised in the database, the login server contacts a launcher instance asking for one.  It then waits and polls the database until it's available, retrying a couple times if necessary.

When the grid/station server is available, the session is serialised and sent to said server.
