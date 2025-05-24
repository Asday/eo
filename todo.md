# TODO

* [ ] miscellanery
  * [ ] prepared statements
  * [ ] DTLS
  * [ ] packet error detection/correction
    * [x] caboose for incomplete message detection
    * [ ] checksum dispersed through the packet for more generic detection
  * [ ] sqlc
    * [ ] handwrite some repo code to get a feel for what's required
    * [ ] author custom template
  * [ ] VM based cluster test environment
    * [ ] get something working
    * [ ] look into cloud-init later
  * [ ] real hardware cluster test environment
  * [ ] logging stuff
    * [ ] enable logging calls based on levels
    * [ ] automatically format messages with their level as a prefix
    * [ ] prefix with executable name, too - looks like terminal output is gonna get mixed
  * [ ] refactor launcher to be a little less insane
    * [ ] multiple files
    * [ ] unique resources instead of custom classes
    * [ ] less const and noexcept
  * [ ] memory management
    * [ ] allocate once at startup and never again
    * [ ] clear delineation between functions that allocate and those that don't
  * [ ] build system (nob style?  `#if 0 /*` stuff like in `cpp-examples`?  MAKE!?)
    * [ ] make for now until it gets ungodlyly unwieldly
      * [ ] incremental
      	* [ ] it should be but isn't for some reason
      * [x] build target only
      * [ ] reduce duplication by listing prerequisite object files in an environment variable then reusing them in the compilation command
    * [ ] my own C++ nob (basically make but in C++ and with niceties)
      * [ ] recompile self
      * [ ] manually define output files
      * [ ] manually define dependency trees
        * [ ] per source file would be nice
        * [ ] ability to output some sort of dotfile to show dependencies maybe?
      * [ ] on invocation build targets listed on the command line
  * [ ] network testing
    * [ ] latency injection
      * [ ] constant
      * [ ] spikes
    * [ ] error injection
      * [ ] incomplete messages
      * [ ] bit flips
      * [ ] consecutive bit corruption
    * [ ] hangups
* [ ] server cluster
  * [ ] instances (long lived stuff)
    * [ ] cluster executable (one per cluster)
      * [ ] connect to database
      * [ ] find launcher instance(s)
      * [ ] launch a login server for own name
      * [ ] launch a chat server
      * [ ] launch persistent services
      * [ ] present CLI
        * [ ] handle cluster shutdown
    * [ ] launcher executable (one per physical machine)
      * [x] connect to database
      * [ ] advertise self in the database
        * [x] ask postgres for IP `inet_client_addr()`
        * [ ] record compute/memory stats
        * [x] store returned server id
      * [x] heartbeat to the database
      * [ ] serve internal requests
        * [x] open a socket
        * [ ] on private WAN
      * [x] wait for orders from cluster
      * [x] launch server instances as requested
      * [ ] destroy server instances as requested
      * [x] clean up heartbeat on shutdown
      * [ ] fix stale tail bug
      * [ ] swap visit lambda to cout a variant for a straight cout if poss
      * [ ] refactor to multiple files
      * [ ] swallow the `^C` in the terminal
      * [ ] spawned processes appear to share stdout, fix that
    * [ ] login server
      * [ ] connect to database for cluster name
      * [ ] advertise IP in the database
      * [ ] accept connections from clients
      * [ ] authenticate
        * [ ] dude just plaintext the passwords for now I DO not care
        * [ ] send a token back to the client to use in future requests
        * [ ] don't even encrypt the packets lmao
      * [ ] send character list
      * [ ] listen for character selection
        * [ ] hand connection over to grid or station server
          * [ ] build session
          * [ ] launch a grid or station server if required
        * [ ] authorise connection to chat server
    * [ ] station server
      * [ ] accept connection handovers
      * [ ] handle undocking
        * [ ] hand connection over to grid server
      * [ ] trade
    * [ ] grid server
      * [ ] accept connection handovers
      * [ ] tick handling
        * [ ] physics simulation
        * [ ] handle timers
      * [ ] objects in space
      * [ ] containers in space
        * [ ] static containers
          * [ ] jet cans
          * [ ] wrecks
          * [ ] passworded cans
          * [ ] locked/unlocked cans (exploration)
          * [ ] ship maintenance arrays
          * [ ] the PoS deployables that store non-ship items, I forget the name
        * [ ] removal of temporary cans
        * [ ] movable containers
          * [ ] fleet hangars
          * [ ] whatever the ship bays in capitals and the orca are called
        * [ ] jettisoning items
        * [ ] moving items between containers
          * [ ] interaction with inventory system, avoid item dupes for God's sake
        * [ ] range checks
        * [ ] ship maintenance array particulars
          * [ ] launching ships
          * [ ] stowing ships
      * [ ] boarding ships
      * [ ] ejecting
      * [ ] targeting
      * [ ] module activation and deactivation
        * [ ] full cycle only (most things)
        * [ ] partial cycle (mining equipment)
      * [ ] combat
        * [ ] damage
          * [ ] damage types
          * [ ] resistance
          * [ ] AoE
          * [ ] direct damage (turrets)
          * [ ] missile damage
        * [ ] healing
        * [ ] ecm
          * [ ] targeted
          * [ ] AoE
        * [ ] neuts/nos
          * [ ] targeted
          * [ ] AoE
        * [ ] web
          * [ ] targeted
          * [ ] wubble
        * [ ] warp inhibition
          * [ ] jam
          * [ ] scram
          * [ ] dictor bubble
          * [ ] hictor bubble
          * [ ] infinipoint
          * [ ] deployable bubble
        * [ ] ship destruction
      * [ ] movement
        * [ ] subwarp
        * [ ] warp
          * [ ] hand connection over to other grid server if required
          * [ ] ewarp
        * [ ] jump gate
        * [ ] jump bridge
        * [ ] tunnel (blops/titan)
        * [ ] cyno
          * [ ] hand connection over to other grid server
      * [ ] mining
      * [ ] tethering
      * [ ] cloaking
      * [ ] deployables
      * [ ] aggression timers
      * [ ] handle docking
      	* [ ] check weapons timer
        * [ ] hand connection over to station server
      * [ ] handle jumping
      	* [ ] hand connection over to other grid server
      	* [ ] check for polarisation
      * [ ] police
      	* [ ] faction
      	* [ ] CONCORD
    * [ ] chat server
      * [ ] accept authorised connections
      * [ ] handle creation of chat room requests
    * [ ] fleet server
      * [ ] handle pilot joining
      * [ ] handle pilot leaving
      * [ ] handle pilot moving
      * [ ] broadcasts
      * [ ] shared commands (fleet warp)
      * [ ] interact with the chat server for fleet chat?
    * [ ] realtime skill training service
      * [ ] flag sessions for update
      * [ ] notify connected client
      * [ ] skill queue
      * [ ] SP calculation
      * [ ] timers for when the next skill level will happen
      * [x] NO unallocated SP, that shit is cuh-RINGE
        * [x] NO skill injectors
    * [ ] OH YEAH market service!  (Arguably the most important)
      * [ ] sell orders in station with timeout
      * [ ] buy orders in station with timeout and range
    * [ ] reinforce service
    * [ ] signature spawning service
      * [ ] assign signatures to scannables
      * [ ] spawn hacking/archæology/anomalies/mining sites in systems that have room
      * [ ] spawn static wormholes for all w-space
      * [ ] spawn static wormholes on collapse for w-space
      * [ ] spawn dynamic wormholes in systems with room
      * [ ] despawn wormholes on timeout
    * [ ] industry service
    * [ ] space junk cleanup service
    * [ ] insurance timeout service
    * [ ] incursion service
      * [ ] spawn incursions on a timer when there's room
      * [ ] spawn dungeons in incursion systems on a timer
    * [ ] contract server
      * [ ] auction
        * [ ] with buyout
        * [ ] without buyout
      * [ ] courier
        * [ ] wrapping items
        * [ ] collateral
        * [ ] reward
        * [ ] unwrapping
      * [ ] item exchange
      * [ ] delete on timeout and refund items
    * [ ] bounty service
      * [ ] save up bounties
      * [ ] pay out on tick
    * [ ] instance cleanup service
      * [ ] determine unused stations/grids/chat servers
      * [ ] tell the launcher to nupe them
    * [ ] planetary interaction server
  * [ ] features (code reused across instances)
    * [ ] inventory feature
      * [ ] active/inactive inventories (in DB or checked out by a server, updated in RAM, to be saved back later)
      * [ ] cargo scanner interaction
    * [ ] session feature
      * [ ] cache of a player's current bonuses, wormhole polarity, probably other stuff
    * [ ] industry feature
      * [ ] in-station slots
      * [ ] PoS slots
    * [ ] standings feature
    * [ ] fitting feature
    * [ ] insurance feature
    * [ ] scanning feature
      * [ ] calculate scan strength
      * [ ] filter results on probe type
    * [ ] hacking/archæology minigame feature
      * [ ] generate a graph
      * [ ] hide the treasure
      * [ ] calculate click outcomes
      * [ ] handle item usage
      * [ ] 3-slot inventory (non-persistent? skip the DB)
      * [ ] unlock/destroy cans on completion/failure
    * [ ] wormhole network feature
      * [ ] mass calculation
    * [ ] dungeon feature
      * [ ] combat missions
      * [ ] cosmic anomalies
      * [ ] escalations
      * [ ] limited time events
      * [ ] incursion encounters
      * [ ] cosmic sig asteroid/ice belts
      * [ ] wormhole encounters(?)
    * [ ] incursion feature
      * [ ] manage penalties
      * [ ] calculate influence change
      * [ ] destroy incursions on mom death
    * [ ] mission feature
      * [ ] combat (interact with dungeon feature)
      * [ ] industry
      * [ ] mining
      * [ ] logistics
    * [ ] sovreignty feature
    * [ ] new character creation feature
    * [ ] oh yeah wallet feature
    * [ ] corporation / alliance feature
* [ ] client
  * [ ] connect to server
  * [ ] render interface
  * [ ] play audio hoots
  * [ ] Windows
  * [ ] mac (maybe)
* [ ] website
  * [ ] payment (lmao)
