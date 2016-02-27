#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h> 
#include "constant.h"
#include "rpc.h"
using namespace std;

pthread_mutex_t lock;
int binderFD = -1;
int listenFD = -1;

int listenToNewSocket() {
	int socketFD;
	char host[256];
	struct sockaddr_in serverIn;
    struct addrinfo *server;
	struct addrinfo h;
	struct addrinfo *r;
	
	// Clear h and prepare to create socket.
	memset(&h, 0, sizeof(h));
	h.ai_family = AF_UNSPEC;
	h.ai_flags = AI_PASSIVE;
	h.ai_socktype = SOCK_STREAM;
	
	// Get address info given specifications.
	if(getaddrinfo(NULL, "0", &h, &server) != 0) {
		cerr << "Problem getting addrinfo" << endl;
	}
	
	// Create socket, bind, and listen for 5 possible clients.
	r = server;
	socketFD = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
	bind(socketFD, server->ai_addr, server->ai_addrlen);
	listen(socketFD, 5);
	
	// Get hostname and socket name to print.
	gethostname(host, 256);
	socklen_t length = sizeof(serverIn);
	getsockname(socketFD, (struct sockaddr *)&serverIn, &length);
	 
	return socketFD;
}

int connectToSocket(char *address, char* port) {
	int socketFD;
	struct sockaddr_in serverAddress;
	struct hostent *server;
	int portNum = atoi(port);
	
	// Connect to the socket
    socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	// Get the host, address, and connect
	server = gethostbyname(address);
	bzero((char*)&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	bcopy((char*)server->h_addr,(char*)&serverAddress.sin_addr.s_addr,server->h_length);
	serverAddress.sin_port = htons(portNum);
	if(connect(socketFD,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
		cerr << "Connected failed: " << strerror(errno) << endl;
	}
	
	// Return descriptor.
	return socketFD;
}

int rpcInit() {
	
	// Initialize our lock.
	pthread_mutex_init(&lock, NULL);
	
	// Retrieve binder info
	char * address = getenv("BINDER_ADDRESS");
	if(address==NULL) {
		cerr << "Couldn't get binder address: " << strerror(errno) << endl;
		return ADDRESS_ERROR;
	}
	char * port = getenv("BINDER_PORT");
	if(port==NULL) {
		cerr << "Couldn't get port address: " << strerror(errno) << endl;
		return PORT_ERROR;
	}
	
	// Connect to binder.
	binderFD = connectToSocket(address, port);
	if(binderFD < 0) {
		cerr << "Couldn't connect to binder." << endl;
		return CONNECT_BINDER_ERROR;
	}
	
	// Create a socket that accepts client connections.
	listenFD = listenToNewSocket();
	if(listenFD < 0) {
		cerr << "Couldn't listen on new socket for client connections" << endl;
		return LISTEN_CLIENTS_SOCKET_ERROR;
	}
	
	// No errors. Return success.
	return 0;
}

int rpcCall(char* name, int* argTypes, void** args) {
	return 0;
}

int rpcCacheCall(char* name, int* argTypes, void** args) {
	return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
	return 0;
}

int rpcExecute() {
	return 0;
}

int rpcTerminate() {
	return 0;
}