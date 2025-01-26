# TODO

* [ ] miscellanery
  * [ ] prepared statements
  * [ ] DTLS
  * [ ] sqlc (with custom template)
* [ ] server cluster
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
    * [ ] advertise IP in the database
      * [x] ask postgres for IP `inet_client_addr()`
      * [ ] record compute/memory stats
      * [x] store returned server id
    * [ ] heartbeat to the database
    * [ ] serve internal requests
      * [ ] open a socket
      * [ ] on private WAN
    * [ ] wait for orders from cluster
    * [ ] launch server instances as requested
    * [ ] destroy server instances as requested
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
    * [ ] handle docking
      * [ ] hand connection over to station server
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
  * [ ] database server
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
  * [ ] industry service
  * [ ] space junk cleanup service
  * [ ] features
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
      * [ ] assign signatures to scannables
      * [ ] filter results on probe type
    * [ ] hacking/archæology/anomaly feature
      * [ ] spawn hacking/archæology/anomaly sites in systems that have space for them on a timer from a list of applicable sites for that system
    * [ ] hacking/archæology minigame feature
      * [ ] generate a graph
      * [ ] hide the treasure
      * [ ] calculate click outcomes
      * [ ] handle item usage
      * [ ] 3-slot inventory (non-persistent? skip the DB)
      * [ ] unlock/destroy cans on completion/failure
    * [ ] wormhole network feature
      * [ ] spawn static wormholes for all w-space
      * [ ] keep timers for when wormholes will collapse
      * [ ] spawn non-static wormholes in systems that have space for them on a timer from a list of applicable wormholes for that system
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
      * [ ] spawn incursions
      * [ ] manage penalties
      * [ ] calculate influence change
      * [ ] spawn dungeons
      * [ ] destroy incursions on mom death
    * [ ] mission feature
      * [ ] combat (interact with dungeon feature)
      * [ ] industry
      * [ ] mining
      * [ ] logistics
    * [ ] new character creation feature
    * [ ] bounty feature
    * [ ] oh yeah wallet feature
    * [ ] corporation / alliance feature
    * [ ] sovreignty feature
    * [ ] contract feature
      * [ ] auction
        * [ ] with buyout
        * [ ] without buyout
      * [ ] courier
        * [ ] wrapping items
        * [ ] collateral
        * [ ] reward
        * [ ] unwrapping
      * [ ] item exchange
* [ ] client
  * [ ] connect to server
  * [ ] render interface
  * [ ] play audio hoots
  * [ ] Windows
* [ ] website
  * [ ] payment (lmao)
