#ifndef CONSTANT_H
#define CONSTANT_H

//length
#define HOSTNAME_LENGTH 32
#define PORT_LENGTH 4
#define NAME_LENGTH 64

//Server/Binder Message
#define REGISTER 10
#define DUPLICATE_REGISTER 1
#define REGISTER_SUCCESS 0
#define REGISTER_FAILURE -1

//Client/Binder Message
#define LOC_REQUEST 11
#define LOC_SUCCESS 0
#define LOC_FAILURE -2

//Client/Server Message
#define EXECUTE 12
#define EXECUTE_SUCCESS 0
#define EXECUTE_FAILURE -3

//Terminate Messages
#define TERMINATE 13

//RPC Messages
#define ADDRESS_ERROR -4
#define PORT_ERROR -5
#define CONNECT_BINDER_ERROR -6
#define LISTEN_CLIENTS_SOCKET_ERROR -7
#define CONNECT_SERVER_ERROR -8

//Other error
#define MSG_LENGTH_ERROR -9;
#define TCP_INIT_ERROR -10;
#define BINDER_LISTEN_ERROR -11;
#define BINDER_PORT_ERROR -12;
#define GET_MSG_TYPE_ERROR -13;
#define TYPE_ERROR -322



#endif
