#ifndef BINDER_H
#define BINDER_H

#include "string.h"

using namespace std;

struct server_info {
    int server_socket;
    string server_name;
    int port_num;
};

struct function_info {
    string function_name;
    int* argTypes;
    int numArgs;
};

struct Comparer {
  bool operator() (const server_info &s1, const server_info &s2) const {
    return s1.server_name < s2.server_name;
  }
};

int tcpInit(int *n_port, int* socketfd);
int getMsgLength(int socketfd);
int getMsgType(int socketfd);
bool sameArgTypes(int a, int b);
void handle_loc_request(int socketfd, int msg_len);
void handle_register_request(int socketfd, int msg_len);
void handle_terminate_request(int socketfd);


#endif
