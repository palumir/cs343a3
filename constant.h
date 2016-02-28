#ifndef constant_h
#define constant_h

//Type error
#define TYPE_ERROR -322

//length
#define HOSTNAME_LENGTH 32
#define PORT_LENGTH 4
#define NAME_LENGTH 65

//Server/Binder Message
#define REGISTER 10
#define REGISTER_SUCCESS 0
#define REGISTER_FAILURE -1
#define DUPLICATE_REGISTER 1

//Client/Binder Message
#define LOC_REQUEST 11
#define LOC_SUCCESS 0
#define LOC_FAILURE -1

//Client/Server Message
#define EXECUTE 12
#define EXECUTE_SUCCESS 0
#define EXECUTE_FAILURE -1

//Terminate Messages
#define TERMINATE 13

//RPC Messages
#define ADDRESS_ERROR -1
#define PORT_ERROR -2
#define CONNECT_BINDER_ERROR -3
#define LISTEN_CLIENTS_SOCKET_ERROR -4


#endif
