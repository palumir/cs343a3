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
#include <pthread.h> 
#include "constant.h"
#include "rpc.h"
#include <vector>
using namespace std;

// Registered functions.
vector<skeleton> registeredFunctions;
vector<string> functionNames;
vector<int*> argTypesList;

int binderFD = -1;
int listenFD = -1;

int numThreads = 0;

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
	
	// Create socket, bind, and listen for clients.
	r = server;
	socketFD = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
	bind(socketFD, server->ai_addr, server->ai_addrlen);
	listen(socketFD, SOMAXCONN); // unbounded number of connections
	
	// Get hostname and socket name to print.
	gethostname(host, 256);
	socklen_t length = sizeof(serverIn);
	getsockname(socketFD, (struct sockaddr *)&serverIn, &length);
	 
	return socketFD;
}

int connectToSocket(char *address, int port) {
	int socketFD;
	struct sockaddr_in serverAddress;
	struct hostent *server;
	int portNum = port;
	
	// Connect to the socket
        socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	// Get the host, address, and connect
	server = gethostbyname(address);
	bzero((char*)&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	bcopy((char*)server->h_addr,(char*)&serverAddress.sin_addr.s_addr,server->h_length);
	serverAddress.sin_port = htons(portNum);
	if(connect(socketFD,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
		cerr << "Connected failed." << endl;
	}
	
	// Return descriptor.
	return socketFD;
}

int getMsgLength(int socketfd) {
    int msg_len = 0;
    
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

int get_arg_size(int type) {
    int arg_size = -1;

    switch(type) {
          case ARG_CHAR:
            arg_size = sizeof(char);
            break;
          
          case ARG_SHORT:
            arg_size = sizeof(short);
            break;

          case ARG_INT:
            arg_size = sizeof(int);
            break;

          case ARG_LONG:
            arg_size = sizeof(long);
            break;

          case ARG_DOUBLE:
            arg_size = sizeof(double);
            break;

          case ARG_FLOAT:
            arg_size = sizeof(float);
            break;
    }
    
    return arg_size;
}

bool sameArgTypes(int a, int b) {
    bool is_a_scalar = false;
    bool is_b_scalar = false;
    
    //type
    int typea = a >> 16;
    int typeb = b >> 16;
    
    //scalar or vector
    int lengtha = a & ((1 << 16) - 1);
    int lengthb = b & ((1 << 16) - 1);
    if (lengtha == 0) {
        is_a_scalar = true;
    }
    if (lengthb == 0) {
        is_b_scalar = true;
    }
    
    if (typea == typeb) {
        if (is_a_scalar == is_b_scalar) {
            return true;
        }
    }
    
    return false;
}

class argStruct {
	public:
	int functionNumber;
	void **args;
	int socketfd;
};

void * runFunction(void *threadArg) {
	
	numThreads++;
	
	// Cast all the arguments
	argStruct *castArgs = (argStruct*)threadArg;
	
	// Run the function.
	skeleton s = registeredFunctions[castArgs->functionNumber];
	int retVal = s(argTypesList[castArgs->functionNumber], castArgs->args);
	
	// Success
	if(retVal == 0) {
		int exSucc = EXECUTE_SUCCESS;
		send(castArgs->socketfd, &exSucc, sizeof(int), 0);
                
                //send function name
                string f_name = functionNames[castArgs->functionNumber];
                char functionName[NAME_LENGTH];
                int len = f_name.length();
                for(int i = 0; i < len; ++i) {
                  functionName[i] = f_name[i];
                }
                functionName[NAME_LENGTH - 1] = '\0';
                send(castArgs->socketfd, functionName, NAME_LENGTH, 0);

                //send argTypes
                int argTypesLen = 0;
                int num = -1;
                while(num != 0) {
                  num = argTypesList[castArgs->functionNumber][argTypesLen];
                  argTypesLen ++;
                }
                //cerr << "argTypesLen after execution: " << argTypesLen << endl;
                send(castArgs->socketfd, argTypesList[castArgs->functionNumber], argTypesLen * 4, 0);
                //cerr << "argTypesLen after execution: " << argTypesLen << endl;

                // Send args (one by one)
                for(int i = 0; i < argTypesLen - 1; ++i) {
                  //send size of args
                  int temp = (argTypesList[castArgs->functionNumber][i] >> 16) & 15;
                  int arg_size = get_arg_size(temp);
                  int length = argTypesList[castArgs->functionNumber][i] & ((1 << 16) - 1);// array or scalar
                  if(length == 0) {
                    length = 1;
                  }
                  arg_size *= length;
                  send(castArgs->socketfd, &arg_size, sizeof(arg_size), 0);
                  cerr << "arg_size sent after execution: " << arg_size << endl;
                          
                  //send args
		  send(castArgs->socketfd, castArgs->args[i], arg_size, 0);
                  cerr << "args after execution: " << &castArgs->args[i] << endl;
	        }       
                
	}
	else {
		int exFail = EXECUTE_FAILURE;
		send(castArgs->socketfd, &exFail, sizeof(int), 0);
	}
	
	numThreads--;
    return 0;
}

void handle_execute_request(int socketfd, int msg_len) {
    //EXECUTE, name, argTypes, args
    
    //get function name
    char function_name[NAME_LENGTH];
    int recv_function_name_num = recv(socketfd, function_name, NAME_LENGTH, 0);
    if(recv_function_name_num < 0) {
        cerr << "Error: fail to receive name part of execute message" << endl;
        //return -1;
    }
    cerr << "client calls funtion: " << function_name << endl;
    
    //get argTypes
    int msg_len_int = msg_len - NAME_LENGTH;
    int* argType = new int[msg_len_int];
    int recv_argTypes_num = recv(socketfd, argType, msg_len_int * 4, 0);
    if(recv_argTypes_num < 0) {
        cerr << "Error: fail to receive argTypes part of execute message" << endl;
        //return -1;
    }
    for(int i = 0; i < msg_len_int; ++i) {
        cerr << "argTypes receives from client: " << argType[i] << endl;
    }

    //get args
    int num_args = msg_len_int - 1; // the size of argTypes is 1 greater than the size of args
    void** args = new void*[num_args];
    for (int i = 0; i < num_args; ++i) {
        //recv the size of the arg
        int size_of_arg;
        int recv_size_arg = recv(socketfd, &size_of_arg, sizeof(size_of_arg), 0);
        if(recv_size_arg < 0) {
            cerr << "Error: fail to receive size of arg" << endl;
            //return -1;
        }
        cerr << "arg size: " << size_of_arg << endl;

        //recv the arg
        void *arg = operator new(size_of_arg);
        int recv_arg = recv(socketfd, arg, size_of_arg, 0);
        if(recv_arg < 0) {
            cerr << "Error: fail to receive arg" << endl;
            //return -1;
        }
        
        //put arg into args array
        args[i] = arg;
        if(string(function_name) == "f0") {
          printf("result, a0 or b0 in f0 at server before execution is: %d\n", *((int *)(args[i])));
       }
    }
    cerr << "successfully receive function message from client" << endl;
	
    // Check if the function is registered in our DB and then run it if it is.
	int x = 0;
	int foundFunction = -1;
	while(x < (int)functionNames.size()) {
		if(function_name == functionNames[x]) {
                        cerr << "found the function name in the database" << endl;
                        //function overloading
                        for(int i = 0; i < msg_len_int; ++i) {
			    if(!sameArgTypes(argType[i], argTypesList[x][i])) {
                                break; 
                            }
                            //found
                            if(i == msg_len_int - 1) {
                                foundFunction = x;
                                cerr << "found the function in the database" << endl;
                            }
                         }
		}
                if(foundFunction != -1) {
                   break;
                }
		x++;
	}

	// Found the function, execute it in a new pthread.
	if(foundFunction != -1) {
		pthread_t runThread;
		//argStruct *threadArgs = new argStruct();
                argStruct threadArgs;
		threadArgs.functionNumber = foundFunction;
		threadArgs.args = new void*[num_args];
                for (int i = 0; i < num_args; ++i) {
                  threadArgs.args[i] = args[i];
                }              
                if(string(function_name) == "f0") {
                  printf("a0 in f0 at thread before execution is: %d\n", *((int *)(threadArgs.args[1])));
                  printf("b0 in f0 at thread before execution is: %d\n", *((int *)(threadArgs.args[2])));
                }
		threadArgs.socketfd = socketfd;
		pthread_create(&runThread, NULL, &runFunction, (void*)&threadArgs);
	}
        //delete(argType);
        //delete(args);
    
}


int rpcInit() {
	
	// Retrieve binder info
	char * address = getenv("BINDER_ADDRESS");
	if(address==NULL) {
		cerr << "Couldn't get binder address: " << endl;
		return ADDRESS_ERROR;
	}
	char * port = getenv("BINDER_PORT");
	if(port==NULL) {
		cerr << "Couldn't get port address: " << endl;
		return PORT_ERROR;
	}
	
	// Connect to binder.
	binderFD = connectToSocket(address, atoi(port));
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
	
	/* Send length of argTypes.
	int argTypesLen = sizeof(argTypes)/sizeof(int);
	send(binderFD, &argTypesLen, sizeof(argTypesLen), 0);*/

        //Johnson: send length of function name plus the length of argTypes array
        int length = NAME_LENGTH; // length of function name
        int argTypesLen = 0;
        int num = -1;
        while(num != 0) {
          num = argTypes[argTypesLen];
          argTypesLen ++;
        }
        cerr << "argTypesLen: " << argTypesLen << endl;
        for(int i = 0; i < argTypesLen; ++i) {
          cerr << "argTypes: " << argTypes[i] << endl;
        }
        length += argTypesLen; // length of function name + length of argTypes array
        cerr << "msg length: " << length << endl;
        send(binderFD, &length, sizeof(length), 0);
	
	// Send type of request.
	int type = REGISTER;
	send(binderFD, &type, sizeof(type), 0);
	
	// Send hostname.
	char hostName[HOSTNAME_LENGTH];
	gethostname(hostName, HOSTNAME_LENGTH);
	//string host = hostName;
	send(binderFD, &hostName, HOSTNAME_LENGTH, 0);
	
	// Send port.
	struct sockaddr_in sin;
        socklen_t sinlen = sizeof(sin);
        getsockname(listenFD, (struct sockaddr *)&sin, &sinlen);
        int listenPort = ntohs(sin.sin_port);
	send(binderFD, &listenPort, PORT_LENGTH, 0);
	
	// Send function name.
	//string functionName = name;
	send(binderFD, name, NAME_LENGTH, 0);
	
	// Send argTypes.
        // Johnson: argTypesLen * 4 since the size of of each item at argTypes is 4 
	send(binderFD, argTypes, argTypesLen * 4, 0);
	
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
		registeredFunctions.push_back(f);
		argTypesList.push_back(argTypes);
		functionNames.push_back(name);
               
                cerr << "REGISTER_SUCCESS: " << name << endl;
		return REGISTER_SUCCESS;
	}
	
	// Do nothing, it's already registered.
	else if(retCode == DUPLICATE_REGISTER) {

                cerr << "DUPLICATE_REGISTER: " << name << endl;
		return DUPLICATE_REGISTER;
	}
	
	// What are you sending me, yo? ;)
	else {
		return TYPE_ERROR;
	}
	
	return 0;
}

int rpcCall(char* name, int* argTypes, void** args) {
        //client will call this which means we need to make a connection to binder
        int client_binderFD = -1;       

        // Retrieve binder info
	char * address = getenv("BINDER_ADDRESS");
	if(address==NULL) {
		cerr << "Couldn't get binder address: " << endl;
		return ADDRESS_ERROR;
	}

	char * port = getenv("BINDER_PORT");
	if(port==NULL) {
		cerr << "Couldn't get port address: " << endl;
		return PORT_ERROR;
	}
	
	// Connect to binder.
	client_binderFD = connectToSocket(address, atoi(port));
	if(client_binderFD < 0) {
		cerr << "Couldn't connect to binder." << endl;
		return CONNECT_BINDER_ERROR;
	} 


	if(client_binderFD > 0) {
		/* Send length of argTypes.
		int argTypesLen = sizeof(argTypes)/sizeof(int);
		send(binderFD, &argTypesLen, sizeof(argTypesLen), 0);*/

                //Johnson: send length of function name plus the length of argTypes array
                int length = NAME_LENGTH; // length of function name
                int argTypesLen = 0;
                int num = -1;
                while(num != 0) {
                  num = argTypes[argTypesLen];
                  argTypesLen ++;
                }
                cerr << "argTypesLen: " << argTypesLen << endl;
                for(int i = 0; i < argTypesLen; ++i) {
                  cerr << "argTypes: " << argTypes[i] << endl;
                }
                length += argTypesLen; // length of function name + length of argTypes array
                cerr << "msg length: " << length << endl;
                send(client_binderFD, &length, sizeof(length), 0);
		
		// Send type of request.
		int type = LOC_REQUEST;
		send(client_binderFD, &type, sizeof(type), 0);
		
		// Send function name.
		//string functionName = name;
		send(client_binderFD, name, NAME_LENGTH, 0);
		
		// Send argTypes.
		// Johnson: argTypesLen * 4 since the size of of each item at argTypes is 4 
	        send(client_binderFD, argTypes, argTypesLen * 4, 0);
		
		// Receive from binder success or failure.
		int retCode;
		recv(client_binderFD, &retCode, sizeof(retCode), 0);
                
                //close connection to binder when done
		
		if(retCode == LOC_SUCCESS) {
			
			/* Receive server name.
			char * serverName = new char[NAME_LENGTH];
			recv(client_binderFD, serverName, NAME_LENGTH, 0);
                        cerr << "serverName: " << serverName << endl;*/
			
			// Receive server port.
			int serverPort;
			int test = recv(client_binderFD, &serverPort, sizeof(serverPort), 0);
                        if(test < 0) {
                          cerr << "failed to receive serverPort" << endl;
                        }
                        cerr << "serverPort: " << serverPort << endl;

                        //Receive server name.
			char * serverName = new char[NAME_LENGTH];
			recv(client_binderFD, serverName, NAME_LENGTH, 0);
                        cerr << "serverName: " << serverName << endl;
			
			// Get the server's socket file descriptor.
			int serverFD = connectToSocket(serverName, serverPort);

                        //send length
                        send(serverFD, &length, sizeof(length), 0);
			
			// Notify we are sending an execute request.
			int execute = EXECUTE;
			send(serverFD, &execute, sizeof(int), 0);
			
			// Send name
			send(serverFD, name, NAME_LENGTH, 0);
			
			// Send argTypes
			send(serverFD, argTypes, argTypesLen * 4, 0);
			
			// Send args (one by one)
                        for(int i = 0; i < argTypesLen - 1; ++i) {
                          //send size of args
                          int temp = (argTypes[i] >> 16) & 15;
                          int arg_size = get_arg_size(temp);
                          int length = argTypes[i] & ((1 << 16) - 1);// array or scalar
                          if(length == 0) {
                            length = 1;
                          }
                          arg_size *= length;
                          send(serverFD, &arg_size, sizeof(arg_size), 0);
                          cerr << "arg_size sent: " << arg_size << endl;
                          
                          //send args
			  send(serverFD, args[i], arg_size, 0);
                          if(string(name) == "f0") {
                            printf("result in f0 before execution is: %d\n", *((int *)(args[0])));
                            printf("a0 in f0 before execution is: %d\n", *((int *)(args[1])));
                            printf("b0 in f0 before execution is: %d\n", *((int *)(args[2])));
                          }
			} 
                        
			// Receive the return value.
			int exRetCode;
			recv(serverFD, &exRetCode, sizeof(int), 0);
                      
                        //send excute result
                        if(exRetCode == EXECUTE_SUCCESS) {
                            //get function name
                            char function_name[NAME_LENGTH];
                            int recv_function_name_num = recv(serverFD, function_name, NAME_LENGTH, 0);
                            if(recv_function_name_num < 0) {
                              cerr << "Error: fail to receive name part of execute success message" << endl;
                              //return -1;
                            }
                            cerr << "funtion is executed: " << function_name << endl;
    
                            //get argTypes
                            int msg_len_int = argTypesLen;
                            int* argType = new int[msg_len_int];
                            int recv_argTypes_num = recv(serverFD, argType, msg_len_int * 4, 0);
                            if(recv_argTypes_num < 0) {
                              cerr << "Error: fail to receive argTypes part of execute success message" << endl;
                              //return -1;
                            }
                            for(int i = 0; i < msg_len_int; ++i) {
                              cerr << "argTypes: " << argType[i] << endl;
                            }

                            //get args
                            int num_args = msg_len_int - 1; // the size of argTypes is 1 greater than the size of args
                            void** returned_args = new void*[num_args];
                            for (int i = 0; i < num_args; ++i) {
                              //recv the size of the arg
                              int size_of_arg;
                              int recv_size_arg = recv(serverFD, &size_of_arg, sizeof(size_of_arg), 0);
                              if(recv_size_arg < 0) {
                                 cerr << "Error: fail to receive size of arg" << endl;
                                 //return -1;
                              }
                              cerr << "arg size: " << size_of_arg << endl;

                              //recv the arg
                              void *arg = operator new(size_of_arg);
                              int recv_arg = recv(serverFD, arg, size_of_arg, 0);
                              if(recv_arg < 0) {
                                 cerr << "Error: fail to receive arg" << endl;
                                  //return -1;
                               }
        
                               //put arg into args array
                               returned_args[i] = arg;
                               cerr << "arg: " << &args[i] << endl;
                         }
                          cerr << "successfully receive execute success message" << endl;

                          //record the result
                          for(int i = 0; i < num_args; ++i) {
                               args[i] = returned_args[i];
                          }

                          delete(argType);
                          delete(returned_args);
                          return EXECUTE_SUCCESS;
                        }
                        else if (exRetCode == EXECUTE_FAILURE) {
                          return EXECUTE_FAILURE;
                        }
			
                        //Johnson: close connection when done
                        close(serverFD);
                        delete(serverName);

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

        //close connection when done
        close(client_binderFD);

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
	bool terminate = false;
    
    while (!terminate) {
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
                    cerr << "msg_len: " << msg_len << endl;
                    
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
                    cerr << "msg_type: " << msg_type << endl;
                    
                    switch (msg_type) {
                            case EXECUTE:
                              handle_execute_request(i, msg_len);
                              break;
							
                            case TERMINATE:
			      while(numThreads>0);
			      terminate = true;
			      break;
                    }
                }
            }
	    if(terminate) break;
        }
    }
       
    //close all the connections
      
    return 0;
}

int rpcTerminate() {
        int terminateFD = -1;
       
        // Retrieve binder info
	char * address = getenv("BINDER_ADDRESS");
	if(address==NULL) {
		cerr << "Couldn't get binder address: " << endl;
		return ADDRESS_ERROR;
	}

	char * port = getenv("BINDER_PORT");
	if(port==NULL) {
		cerr << "Couldn't get port address: " << endl;
		return PORT_ERROR;
	}
	
	// Connect to binder.
	terminateFD = connectToSocket(address, atoi(port));
	if(terminateFD < 0) {
		cerr << "Couldn't connect to binder." << endl;
		return CONNECT_BINDER_ERROR;
	} 
	
	// Terminate if we are connected.
	if(terminateFD > 0) {
		int terminate = TERMINATE;
		send(terminateFD, &terminate, sizeof(terminate), 0);
		return 0;
	}
	
	// Don't explode if we're not connected.
	return 0;
}
