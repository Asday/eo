# Inventories

A collection of items in the same container is managed by the inventory system.  Inventories in use can need to be modified very quickly such as small blaster ammunition usage, or modified in great bulk such as a reprocess operation.  Inventories not in use need to be persisted to the database.

An inventory must be checked out of the database to be loaded into a process' memory.  This involves marking the inventory as checked out by a server ID, and resaving the contents when checking the inventory back in.

Items in inventories must not be duplicated in the case of node crashes or trade operations.  Any operation that moves items from one inventory to another, or creates items, must be serialised to the database with a checkin/checkout procedure.
