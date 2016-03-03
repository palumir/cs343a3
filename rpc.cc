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

// Registered functions.
int numRegistered = 0;
skeleton registeredFunctions[256];
string functionNames[256];
int[] argTypesList[256];


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

int getMsgLength(int socketfd) {
    int msg_len;
    
    int length_num = recv(socketfd, &msg_len, sizeof(msg_len), 0);
    if(length_num < 0) {
        cerr << "Error: fail to receive message length" << endl;
        return -1;
    }
    
    return msg_len;
}

int getMsgType(int socketfd) {
    int msg_type;
    
    int type_num = recv(socketfd, &msg_type, sizeof(msg_type), 0);
    if(type_num < 0) {
        cerr << "Error: fail to receive message type" << endl;
        return -1;
    }
    
    return msg_type;
}

void handle_execute_request(int socketfd, int msg_len) {
    //EXECUTE, name, argTypes, args
    
    //get function name
    char function_name[NAME_LENGTH];
    int recv_function_name_num = recv(socketfd, function_name, NAME_LENGTH, 0);
    if(recv_function_name_num < 0) {
        cerr << "Error: fail to receive name part of loc_requst message" << endl;
        //return -1;
    }
    
    //get argTypes
    int msg_len_int = msg_len - NAME_LENGTH;
    int argType[msg_len_int / 4];// divide by 4 since the size of each int in argTypes is 4 bytes
    int recv_argTypes_num = recv(socketfd, argType, msg_len_int, 0);
    if(recv_argTypes_num < 0) {
        cerr << "Error: fail to receive argTypes part of loc_requst message" << endl;
        //return -1;
    }

    //get args
    int num_args = msg_len_int / 4 - 1; // the size of argTypes is 1 greater than the size of args
    void** args = new void*[num_args];
    for (int i = 0; i == num_args; ++i) {
        //recv the size of the arg
        int size_of_arg;
        int recv_size_arg = recv(socketfd, &size_of_arg, sizeof(size_of_arg), 0);
        if(recv_size_arg < 0) {
            cerr << "Error: fail to receive size of arg" << endl;
            //return -1;
        }

        //recv the arg
        void *arg = (void *)malloc(size_of_arg); // c++ is not allowed to use new for void so use malloc
        int recv_arg = recv(socketfd, arg, size_of_arg, 0);
        if(recv_arg < 0) {
            cerr << "Error: fail to receive arg" << endl;
            //return -1;
        }
        
        //put arg into arg array
        args[i] = arg;
    }
    //compare function names
    //compare argTypes of the function
    //compare args of the function
    
    //if found the function is in database, create a thread and execute it
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

int rpcRegister(char* name, int* argTypes, skeleton f) {
	
	// Send length of argTypes.
	int argTypesLen = argTypes.length();
	send(binderFD, &argTypesLen, sizeof(argTypesLen), 0);
	
	// Send type of request.
	int type = REGISTER;
	send(binderFD, &type, sizeof(type), 0);
	
	// Send hostname.
	char hostName[HOSTNAME_LENGTH];
	gethostname(hostName, HOSTNAME_LENGTH);
	string host = hostName;
	send(binderFD, &host, HOSTNAME_LENGTH, 0);
	
	// Send port.
	struct sockaddr_in sin;
    socklen_t sinlen = sizeof(sin);
    getsockname(listenFD, (struct sockaddr *)&sin, &len);
    int listenPort = ntohs(sin.sin_port);
	send(binderFD, &listenPort, PORT_LENGTH, 0);
	
	// Send function name.
	string functionName = name;
	send(binderFD, &functionName, NAME_LENGTH, 0);
	
	// Send argTypes.
	send(binderFD, &argTypes, msgLength, 0);
	
	// Receive from binder success or failure.
	int retCode;
	recv(binderFD, &retCode, sizeof(retCode), 0);
	
	// We failed for some reason.
	if(retCode == REGISTER_FAILURE) {
		cerr << "Failure registering function: " << name << endl;
		return REGISTER_FAILURE;
	}
	
	// Register success. Do stuff.
	else if(retCode == REGISTER_SUCCESS) {
		registeredFunctions[numRegistered] = f;
		argTypesList[numRegistered] = argTypes;
		functionNames[numRegistered] = name;
		numRegistered++;
		return REGISTER_SUCCESS;
	}
	
	// Do nothing, it's already registered.
	else if(retCode == DUPLICATE_REGISTER) {
		return DUPLICATE_REGISTER;
	}
	
	// What are you sending me, yo? ;)
	else {
		return TYPE_ERROR;
	}
	
	return 0;
}

int rpcCall(char* name, int* argTypes, void** args) {
	if(binderFD > 0) {
		// Send length of argTypes.
		int argTypesLen = argTypes.length();
		send(binderFD, &argTypesLen, sizeof(argTypesLen), 0);
		
		// Send type of request.
		int type = LOC_REQUEST;
		send(binderFD, &type, sizeof(type), 0);
		
		// Send function name.
		string functionName = name;
		send(binderFD, &functionName, NAME_LENGTH, 0);
		
		// Send argTypes.
		send(binderFD, &argTypes, msgLength, 0);
		
		// Receive from binder success or failure.
		int retCode;
		recv(binderFD, &retCode, sizeof(retCode), 0);
		
		if(retCode == LOC_SUCCESS) {
			
			// Receive server name.
			char * serverName = new char[NAME_LENGTH];
			ret = recv(x, serverName, NAME_LENGTH, 0);
			
			// Receive server port.
			int serverPort;
			recv(binderFD, &serverPort, PORT_LENGTH, 0);
			
			// Get the server's socket file descriptor.
			int serverFD = connectToSocket(serverName, itoa(serverPort));
			
			// SEND EXECUTE REQUEST TO SERVER WIP (WORK IN PROGRESS)
			int exRetCode = 0; // SendExecuteRequest WIP
			
			return exRetCode;
		}
		
		if(retCode == LOC_FAILURE) {
			cerr << "rpcCall location failure." << endl;
			return LOC_FAILURE;
		}
		
		else {
			return TYPE_ERROR;
		}
	}
	return 0;
}

int rpcCacheCall(char* name, int* argTypes, void** args) {
	return 0;
}

int rpcExecute() {
	//wait for connection from client (unbounded)
    int listen_num = listen(listenFD, SOMAXCONN);
    if (listen_num != 0) {
        close(listenFD);
        cerr << "Error: fail to listen to server" << endl;
        return -1;
    }
    
    
    //select
    //ref: http://www.lowtek.com/sockets/select.html
    //ref: http://www.cs.toronto.edu/~krueger/csc209h/f05/lectures/Week11-Select.pdf
    //ref: https://en.wikipedia.org/wiki/Select_(Unix)
    //ref: http://www.linuxhowtos.org/C_C++/socket.htm
    fd_set rfds;
    fd_set rset; //descriptor sets
    FD_ZERO(&rfds);
    FD_ZERO(&rset);
    FD_SET(listenFD, &rset);
    
    int highsock = listenFD;
    int readsocks;
    
    while (1) {
        memcpy(&rfds, &rset, sizeof(rset));
        readsocks = select(highsock + 1, &rfds, (fd_set *) 0, (fd_set *) 0, NULL);
        
        if(readsocks < 0) {
            cerr << "failed to select" << endl;
            //return -1;
        }
        
        for(int i = 0; i < highsock + 1; ++i) {
            if(FD_ISSET(i, &rfds)) {
                //client tries to connect
                if(i == listenFD) {
                    struct sockaddr_in cli_addr;
                    socklen_t clilen = sizeof(cli_addr);
                    int connectionfd = accept(listenFD, (struct sockaddr *) &cli_addr, &clilen);
                    if(connectionfd < 0) {
                        cerr << "failed to accept connection from server" << endl;
                    }
                    
                    cerr << "connected to client" << endl;
                    //update the descriptor sets
                    FD_SET(connectionfd, &rset);
                    if(connectionfd > highsock) {
                        highsock = connectionfd;
                    }
                }
                //handle request
                else {
                    //get message length (4 bytes)
                    int msg_len = getMsgLength(i);
                    if (msg_len == -1) {
                        return -1;
                    }
                    
                    //msg_len == 0 means client tries to end connection
                    if(msg_len == 0) {
                        FD_CLR(i, &rset);
                        close(i);
                        continue;
                    }
                    
                    //get message type (4 bytes)
                    int msg_type = getMsgType(i);
                    if (msg_type == -1) {
                        return -1;
                    }
                    
                    switch (msg_type) {
                            case EXECUTE:
                            handle_execute_request(i, msg_len);
                            break;
                    }
                }
            }
        }
    }
       
    //close all the connections
      
    return 0;
}

int rpcTerminate() {
	
	// Terminate if we are connected.
	if(binderFD > 0) {
		cerr << "Couldn't connect to binder." << endl;
		int terminate = TERMINATE;
		send(binderFD, &terminate, sizeof(terminate), 0);
		return 0;
	}
	
	// Don't explode if we're not connected.
	return 0;
}
