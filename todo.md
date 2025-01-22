# TODO

* [ ] server cluster
  * [ ] cluster executable
    * [ ] connect to database
    * [ ] advertise IP in the database
    * [ ] wait for workers to connect
    * [ ] launch a login server
  * [ ] cluster worker executable
    * [ ] connect to cluster executable
    * [ ] launch server instances as requested
    * [ ] destroy server instances as requested
  * [ ] login server
    * [ ] accept connections from clients
    * [ ] authenticate
    * [ ] display character list and allow selection
      * [ ] hand connection over to grid or station server
      * [ ] authorise connection to chat server
  * [ ] station server
    * [ ] accept connection handovers
    * [ ] handle undocking
      * [ ] hand connection over to grid server
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
  * [ ] systems (how do these work and interact with servers?)
    * [ ] inventory system
      * [ ] active/inactive inventories (in DB or checked out by a server, updated in RAM, to be saved back later)
      * [ ] cargo scanner interaction
    * [ ] session system
      * [ ] cache of a player's current bonuses, wormhole polarity, probably other stuff
    * [ ] industry system
      * [ ] in-station slots
      * [ ] PoS slots
    * [ ] standings system
    * [ ] fitting system
    * [ ] insurance system
    * [ ] scanning system
      * [ ] calculate scan strength
      * [ ] assign signatures to scannables
      * [ ] filter results on probe type
    * [ ] hacking/archæology/anomaly system
      * [ ] spawn hacking/archæology/anomaly sites in systems that have space for them on a timer from a list of applicable sites for that system
    * [ ] hacking/archæology minigame system
      * [ ] generate a graph
      * [ ] hide the treasure
      * [ ] calculate click outcomes
      * [ ] handle item usage
      * [ ] 3-slot inventory system (non-persistent? skip the DB)
      * [ ] unlock/destroy cans on completion/failure
    * [ ] wormhole network system
      * [ ] spawn static wormholes for all w-space
      * [ ] keep timers for when wormholes will collapse
      * [ ] spawn non-static wormholes in systems that have space for them on a timer from a list of applicable wormholes for that system
      * [ ] mass calculation
    * [ ] dungeon system
      * [ ] combat missions
      * [ ] cosmic anomalies
      * [ ] escalations
      * [ ] limited time events
      * [ ] incursion encounters
      * [ ] cosmic sig asteroid/ice belts
      * [ ] wormhole encounters(?)
    * [ ] incursion system
      * [ ] spawn incursions
      * [ ] manage penalties
      * [ ] calculate influence change
      * [ ] spawn dungeons
      * [ ] destroy incursions on mom death
    * [ ] mission system
      * [ ] combat (interact with dungeon system)
      * [ ] industry
      * [ ] mining
      * [ ] logistics
    * [ ] new character creation system
    * [ ] bounty system
    * [ ] realtime skill training system
      * [ ] flag sessions for update
      * [ ] notify connected client
      * [ ] skill queue
      * [ ] SP calculation
      * [ ] timers for when the next skill level will happen
      * [x] NO unallocated SP, that shit is cuh-RINGE
        * [x] NO skill injectors
    * [ ] oh yeah wallet system
    * [ ] corporation / alliance system
    * [ ] sovreignty system
    * [ ] OH YEAH market system!  (Arguably the most important)
      * [ ] sell orders in station with timeout
      * [ ] buy orders in station with timeout and range
    * [ ] contract system
      * [ ] auction
        * [ ] with buyout
        * [ ] without buyout
      * [ ] courier
        * [ ] wrapping items
        * [ ] collateral
        * [ ] reward
        * [ ] unwrapping
      * [ ] item exchange
    * [ ] trade system
* [ ] client
  * [ ] connect to server
  * [ ] render interface
  * [ ] play audio hoots
  * [ ] Windows
