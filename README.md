# Simple-HTTP-Server
A project for COMP30023: Computer Systems at the University of Melbourne.
Server can receive, process and respond to simple HTTP GET requests.
To run the server, after compiling (server.c and httplib.h), use the command:
./server [IvP protocol number] [port number] [string path to root web directory]
  Where the protocol number is either 4 for IvP4, or 6 for IvP6.
  The port number is a valid port number to operate on.
  The root web directory is the relative file directory the server will search for the requested file requested in received GET requests.

