# Security

IPC and orchestration is done over unencrypted UDP (and maybe sometimes TCP) within a VPC.  The database also lives there, so no password or anything.

External connections are UDP with DTLS.

State is managed by the servers and transferred internally, the external game clients bring only their DTLS communications.
