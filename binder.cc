#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>
#include <netdb.h>
#include "constant.h"
#include "binder.h"

using namespace std;

vector<int> all_servers;
map<server_info, vector<function_info>> database;

int tcpInit(int *n_port, int* socketfd) {
    //initiate socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      cerr << "Error: opening socket" << endl;
      return -1;
    }
    
    //initiate sockaddr structure
    struct sockaddr_in ser_address;
    
    ser_address.sin_family = AF_INET;
    ser_address.sin_port = 0;
    ser_address.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //try to bind
    int bnum = bind(sockfd, (struct sockaddr*) &ser_address, sizeof(ser_address));
    if (bnum < 0) {
      cerr << "err: unable to bind" << endl;
    }
    
    *socketfd = sockfd;
    return 0;
}


int getMsgLength(int socketfd) {
    char msg_length[4] = {0};
    int msg_len = 0;
    
    int length_num = recv(socketfd, msg_length, 4, 0);
    if(length_num < 0) {
      cerr << "Error: fail to receive string length from client" << endl;
        //return -1;
    }
    //cout << "get mssage length: " << req_msg[0] + req_msg[1] + req_msg[2] + req_msg[3] << endl;
    
    
    msg_len = (msg_length[0] << 24) + (msg_length[1] << 16) + (msg_length[2] << 8) + msg_length[3];
    
    return msg_len;
}

int getMsgType(int socketfd) {
    char msg_type_char[4] = {0};
    
    int msg_type = 0;
    
    int type_num = recv(socketfd, msg_type_char, 4, 0);
    if(type_num < 0) {
        cerr << "Error: fail to receive string length from client" << endl;
        //return -1;
    }
    //cout << "get mssage length: " << req_msg[0] + req_msg[1] + req_msg[2] + req_msg[3] << endl;
    
    
    msg_type = (msg_type_char[0] << 24) + (msg_type_char[1] << 16) + (msg_type_char[2] << 8) + msg_type_char[3];
    
    return msg_type;
}

void handle_loc_request(int socketfd, int msg_len) {
    //LOC_REQUEST, name, argTypes
    
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
    
    //search at the database with function name and argTypes
    
    //send the server location when we have successful search
    
}

void handle_register_request(int socketfd, int msg_len) {
    //REGISTER, server_identifier, port, name, argTypes
    
    //get server_identifier
    char server_identifier[HOSTNAME_LENGTH];
    int recv_server_identifier_num = recv(socketfd, server_identifier, HOSTNAME_LENGTH, 0);
    if(recv_server_identifier_num < 0) {
        cerr << "Error: fail to receive name part of loc_requst message" << endl;
        //return -1;
    }

    
    //get port
    char port[PORT_LENGTH];
    int recv_port_num = recv(socketfd, port, PORT_LENGTH, 0);
    if(recv_server_identifier_num < 0) {
        cerr << "Error: fail to receive name part of loc_requst message" << endl;
        //return -1;
    }
    
    //get name
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
    
    server_info server_location;
    server_location.server_socket = socketfd;
    server_location.server_name = string(server_identifier);
    server_location.port_num = (port[0] << 24) + (port[1] << 16) + (port[2] << 8) + port[3];;
    
    function_info ftn;
    ftn.function_name = string(function_name);
    ftn.argTypes = argType;
    
    
    //add to database
    database[server_location].push_back(ftn);
    
    
    //send successful message
    char return_message[4];
    return_message[0] = (REGISTER_SUCCESS >> 24) & 0xFF;
    return_message[1] = (REGISTER_SUCCESS >> 16) & 0xFF;
    return_message[2] = (REGISTER_SUCCESS >> 8) & 0xFF;
    return_message[3] = REGISTER_SUCCESS & 0xFF;
    int sent = send(socketfd, return_message, 4, 0);
    if (sent < 0) {
        cerr << "Error: fail to send the message" << endl;
    }
}

/*void recvMsg(int socketfd, unsigned int msg_len, unsigned char** msg) {
    int counter = (int) msg_len;
    unsigned int upto = 0;
    while(counter > 0) {
      unsigned char *req_msg = new unsigned char[msg_len];
      int recv_num = recv(socketfd, req_msg, msg_len, 0);
      //cout << "recv_num: " << recv_num << endl;
      if(recv_num < 0) {
        cerr << "Error: fail to receive string from client" << endl;
        //return -1;
      }
      counter -= recv_num;
        
      int i = 0;
      for (unsigned int k = upto; k < recv_num + upto; ++k) {
        *msg[k] = req_msg[i];
        //cout << "msg[" << k << "]: " << msg[k] << endl;
        i ++;
       }
       upto += recv_num;
       delete(req_msg);
    }
    
    unsigned int a = msg_len - 1;
    *msg[a] = '\0';
}

void unmarshall() {
    
}*/



int main(int argc, const char* argv[]) {
    int socketfd;
    int n_port;
    
    //create socket
    int tcp = tcpInit(&n_port, &socketfd);
    if (tcp != 0) {
      return -1;
    }
    
    //wait for connection (unbounded)
    int listen_num = listen(socketfd, SOMAXCONN);
    if (listen_num != 0) {
      close(socketfd);
      cerr << "Error: fail to listen to socketfd" << endl;
      return -1;
    }
    
    //get machine name
    //ref:http://stackoverflow.com/questions/504810/how-do-i-find-the-current-machines-full-hostname-in-chostname-and-domain-info
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    cout << "BINDER_ADDRESS " << hostname << endl;
    
    //get binder port
    //ref: http://stackoverflow.com/questions/4046616/sockets-how-to-find-out-what-port-and-address-im-assigned
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socketfd, (struct sockaddr *)&sin, &len) == -1) {
      cerr << "err: unable to get server port" << endl;
      return -1;
    }
    else {
      cout << "BINDER_PORT " << ntohs(sin.sin_port) << endl;
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
    FD_SET(socketfd, &rset);
    
    int highsock = socketfd;
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
          //client tries to connect, server will accept
          if(i == socketfd) {
            struct sockaddr_in cli_addr;
            socklen_t clilen = sizeof(cli_addr);
            int connectionfd = accept(socketfd, (struct sockaddr *) &cli_addr, &clilen);
            if(connectionfd < 0) {
              cerr << "failed to accept connection from client" << endl;
            }
                    
            cerr << "connected to client" << endl;
            //update the descriptor sets
            FD_SET(connectionfd, &rset);
            if(connectionfd > highsock) {
              highsock = connectionfd;
            }
            
            //add connection to list of server
            all_servers.push_back(i);
          }
          //handle request
          else {
            //get message length (4 bytes)
            int msg_len = getMsgLength(i);
              
            //get message type (4 bytes)
            int msg_type = getMsgType(i);
              
              switch (msg_type) {
                  case LOC_REQUEST:
                      //unsigned int actual_len = msg_len - 8;
                      handle_loc_request(i, msg_len);
                      break;
                      
                  case REGISTER:
                      handle_register_request(i, msg_len);
                      break;
                
                  case TERMINATE:
                      break;
              }
              
            //msg_len == 0 means client/server tries to end connection
            if(msg_len == 0) {
              FD_CLR(i, &rset);
              close(i);
              continue;
            }
            
            //receives request message
            //unsigned char *msg = new unsigned char[msg_len];
            //recvMsg(i, msg_len, &msg);
            
            //unmarshall message
            //unmarshall();
          }
        }
          
      }
    }

    
    return 0;
}
