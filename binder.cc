#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>
#include <netdb.h>
#include <algorithm>
#include "constant.h"
#include "binder.h"

using namespace std;

vector<int> servers_socket;
vector<server_info> all_servers;
map<server_info, vector<function_info>, Comparer> binder_database;
bool all_close = false;

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
      cerr << "Error: unable to bind" << endl;
    }
    
    *socketfd = sockfd;
    return 0;
}


int getMsgLength(int socketfd) {
    //char msg_length[4] = {0};
    int msg_len;
    
    int length_num = recv(socketfd, &msg_len, sizeof(msg_len), 0);
    if(length_num < 0) {
      cerr << "Error: fail to receive message length" << endl;
      return -1;
    }
    //cout << "get mssage length: " << req_msg[0] + req_msg[1] + req_msg[2] + req_msg[3] << endl;
    
    
    //msg_len = (msg_length[0] << 24) + (msg_length[1] << 16) + (msg_length[2] << 8) + msg_length[3];
    
    return msg_len;
}

int getMsgType(int socketfd) {
    //char msg_type_char[4] = {0};
    
    int msg_type;
    
    int type_num = recv(socketfd, &msg_type, sizeof(msg_type), 0);
    if(type_num < 0) {
        cerr << "Error: fail to receive message type" << endl;
        return -1;
    }
    //cout << "get mssage length: " << req_msg[0] + req_msg[1] + req_msg[2] + req_msg[3] << endl;
    
    
    //msg_type = (msg_type_char[0] << 24) + (msg_type_char[1] << 16) + (msg_type_char[2] << 8) + msg_type_char[3];
    
    return msg_type;
}

bool sameArgTypes(int a, int b) {
    bool is_a_scalar = false;
    bool is_b_scalar = true;
    
    //input or output
    int ioa = a >> 30;
    int iob = b >> 30;
    
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
    
    if (ioa == iob && typea == typeb) {
        if (is_a_scalar == is_b_scalar) {
            return true;
        }
    }
    
    return false;
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
    bool found = false;
    string server_name_result;
    int server_port_result = -10;
    string function_name_request = string(function_name);
    
    //for round robin
    vector<string> all_valid_server; //all servers that owns the function
  
    for (map<server_info, vector<function_info> >::iterator it = binder_database.begin();
         it != binder_database.end(); ++it) {
        vector<function_info> temp = it->second;
        for (vector<function_info>::iterator i = temp.begin();
             i != temp.end(); ++i) {
            //compare function name
            string function_name_temp = i->function_name;
            if (function_name_temp == function_name_request) {
                //compare args
                int totalArgs = i->numArgs;
                for(int j = 0; j < totalArgs; ++j) {
                    if (!sameArgTypes(i->argTypes[j], argType[j])) {
                        break;
                    }
                    else if (j == totalArgs - 1) {
                        all_valid_server.push_back(it->first.server_name);
                        found = true;
                    }
                }
            }
        }
    }
    
    bool finished_round_robin = false;
    server_info temp_server;
    if (found) {
        for(vector<server_info>::iterator it = all_servers.begin();
            it != all_servers.end(); ++it) {
            for (vector<string>::iterator i = all_valid_server.begin();
                 i != all_valid_server.end(); ++i) {
                string v_server = *i;
                if (it->server_name == v_server) {
                  server_name_result = it->server_name;
                  server_port_result = it->port_num;
                
                  //delete server from the list first
                  all_servers.erase(it);
                
                  //then put it at the back in order to do round robin
                  temp_server.server_name = it->server_name;
                  temp_server.server_socket = it->server_socket;
                  temp_server.port_num = it->port_num;
                
                  all_servers.push_back(temp_server);
                
                  finished_round_robin = true;
                  break;
                }
            }
            if(finished_round_robin) {
                break;
            }
        }
    }

    
    //send the server location when we have successful search
    if (finished_round_robin) {
        //send success message
        int success = LOC_SUCCESS;
        int send_success_message = send(socketfd, &success, sizeof(success), 0);
        if (send_success_message < 0) {
            cerr << "Error: fail to send success message when handle the loc_req message" << endl;
        }

        
        //send server_name
        char server_name[HOSTNAME_LENGTH];
        strcpy(server_name, server_name_result.c_str());
        server_name[HOSTNAME_LENGTH - 1] = '\0';
        int send_server_name = send(socketfd, server_name, HOSTNAME_LENGTH, 0);
        if (send_server_name < 0) {
            cerr << "Error: fail to send the server name when handle the loc_req message" << endl;
        }
        
        //send port_num
        int port = server_port_result;
        int send_port_num = send(socketfd, &port, sizeof(port), 0);
        if (send_port_num < 0) {
            cerr << "Error: fail to send the port num when handle the loc_req message" << endl;
        }
    }
    else {
        //send failure message
        int failure = LOC_FAILURE;
        int send_failure_message = send(socketfd, &failure, sizeof(failure), 0);
        if (send_failure_message < 0) {
            cerr << "Error: fail to send the failure message when handle the loc_req message" << endl;
        }
    }
    
}

void handle_register_request(int socketfd, int msg_len) {
    //REGISTER, server_identifier, port, name, argTypes
    
    //get server_identifier
    char server_identifier[HOSTNAME_LENGTH];
    int recv_server_identifier_num = recv(socketfd, server_identifier, HOSTNAME_LENGTH, 0);
    if(recv_server_identifier_num < 0) {
        cerr << "Error: fail to receive server identifier of reg_requst message" << endl;
        //return -1;
    }

    
    //get port
    //char port[PORT_LENGTH];
    int port;
    int recv_port_num = recv(socketfd, &port, PORT_LENGTH, 0);
    if(recv_port_num < 0) {
        cerr << "Error: fail to receive port number of reg_requst message" << endl;
        //return -1;
    }
    
    //get function name
    char function_name[NAME_LENGTH];
    int recv_function_name_num = recv(socketfd, function_name, NAME_LENGTH, 0);
    if(recv_function_name_num < 0) {
        cerr << "Error: fail to receive function name of reg_requst message" << endl;
        //return -1;
    }
    
    //get argTypes
    int msg_len_int = msg_len - NAME_LENGTH;
    int argType[msg_len_int / 4];// divided by 4 since the size of each int in argTypes is 4 bytes
    int recv_argTypes_num = recv(socketfd, argType, msg_len_int, 0);
    if(recv_argTypes_num < 0) {
        cerr << "Error: fail to receive argTypes part of reg_requst message" << endl;
        //return -1;
    }
    
    //check whether the server or function is in database
    server_info server_location;
    bool server_exist = false;
    bool function_exist = false;
    for (map<server_info, vector<function_info> >::iterator it = binder_database.begin();
         it != binder_database.end(); ++it) {
        //found server
        if(it->first.server_name == string(server_identifier)) {
            server_exist = true;
            server_location.server_socket = it->first.server_socket;
            server_location.server_name = it->first.server_name;
            server_location.port_num = it->first.port_num;
            
            //check whether the function is in database
            vector<function_info> temp = it->second;
            for (vector<function_info>::iterator i = temp.begin();
                 i != temp.end(); ++i) {
                //compare function name
                string function_name_temp = i->function_name;
                if (function_name_temp == string(function_name)) {
                    //compare args
                    int totalArgs = i->numArgs;
                    for(int j = 0; j < totalArgs; ++j) {
                        if (!sameArgTypes(i->argTypes[j], argType[j])) {
                            break;
                        }
                        else if (j == totalArgs - 1) {
                            function_exist = true;
                        }
                    }
                }
            }
            //send duplicate register message
            if (server_exist && function_exist) {
                int duplicate_register = DUPLICATE_REGISTER;
                int sent_duplicate = send(socketfd, &duplicate_register, sizeof(duplicate_register), 0);
                if (sent_duplicate < 0) {
                    cerr << "Error: fail to send DUPLICATE_REGISTER message" << endl;
                }
                return;
            }
        }
    }
    
    

    if(!server_exist) {
      server_location.server_socket = socketfd;
      server_location.server_name = string(server_identifier);
      server_location.port_num = port;
        
      //add to server list
      all_servers.push_back(server_location);
      servers_socket.push_back(socketfd);
      vector<function_info> new_list_of_function;
      binder_database.insert(make_pair(server_location, new_list_of_function));
    }
    
    function_info ftn;
    if(!function_exist) {
      ftn.function_name = string(function_name);
      ftn.argTypes = new int[msg_len_int / 4];
      memcpy(ftn.argTypes, argType, msg_len_int);
      ftn.numArgs = msg_len_int / 4;
      
      //add function to database
      binder_database[server_location].push_back(ftn);
        
      delete(ftn.argTypes);
    }
    
    
    //see if the function registration has been successful
    bool success = false;
    map<server_info, vector<function_info> >::iterator server_in_DB;
    server_in_DB = binder_database.find(server_location);
    
    if(server_in_DB != binder_database.end()) {
      vector<function_info>::iterator function_in_DB;
      for (function_in_DB = binder_database[server_location].begin();
           function_in_DB != binder_database[server_location].end(); ++function_in_DB) {
          //same function name
          if (function_in_DB->function_name == ftn.function_name) {
              //same numArgs and argTypes
              if(function_in_DB->numArgs == ftn.numArgs) {
                  for (int j = 0; j < ftn.numArgs; ++j) {
                      if (!sameArgTypes(function_in_DB->argTypes[j], ftn.argTypes[j])) {
                          break;
                      }
                      else if (j == ftn.numArgs - 1) {
                          success = true;
                      }
                  }
              }
          }
          if(success) {
              break;
          }
      }
    }
    
    //send result message
    if(success) {
      int success = REGISTER_SUCCESS;
      int send_success = send(socketfd, &success, sizeof(success), 0);
      if (send_success < 0) {
        cerr << "Error: fail to REGISTER_SUCCESS the message" << endl;
      }
    }
    else {
      int failure = REGISTER_FAILURE;
      int send_failure = send(socketfd, &failure, sizeof(failure), 0);
      if (send_failure < 0) {
        cerr << "Error: fail to REGISTER_FAILURE the message" << endl;
      }
    }
}

void handle_terminate_request(int socketfd) {
    //TERMINATE
    
    //get binder's name
    char hostname[HOSTNAME_LENGTH];
    hostname[HOSTNAME_LENGTH - 1] = '\0';
    gethostname(hostname, HOSTNAME_LENGTH);
    
    //inform all the servers to shut down
    for (vector<int>::iterator i = servers_socket.begin();
         i != servers_socket.begin(); ++i) {
         //send terminate message
         int socket_end = *i;
         int terminate = TERMINATE;
         int terminate_message = send(socket_end, &terminate, sizeof(terminate), 0);
         if (terminate_message < 0) {
             cerr << "Error: fail to send terminate message when handle the terminate_req message" << endl;
         }
    }
    
}


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
      cerr << "err: unable to get binder port" << endl;
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
          //client/server tries to connect, binder will accept
          if(i == socketfd) {
            struct sockaddr_in cli_addr;
            socklen_t clilen = sizeof(cli_addr);
            int connectionfd = accept(socketfd, (struct sockaddr *) &cli_addr, &clilen);
            if(connectionfd < 0) {
              cerr << "failed to accept connection" << endl;
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
              
            //msg_len == 0 means server tries to end connection
            if(msg_len == 0) {
              FD_CLR(i, &rset);
              close(i);
                
              //remove from server list
              servers_socket.erase(remove(servers_socket.begin(), servers_socket.end(), i), servers_socket.end());
                
              for (vector<server_info>::iterator it = all_servers.begin();
                   it != all_servers.end(); ++it) {
                   if(it->server_socket == i) {
                      all_servers.erase(it);
                   }
              }
              
              
              //remove entry of closed server from database
              for (map<server_info, vector<function_info> >::iterator it = binder_database.begin();
                   it != binder_database.end(); ++it) {
                   if (it->first.server_socket == i) {
                      binder_database.erase(it);
                   }
              }
                
              //close binder after termination of all servers
              if (all_close && binder_database.empty() &&
                  servers_socket.empty() && all_servers.empty()) {
                  return 0;
              }
              continue;
            }
              
              
            //get message type (4 bytes)
            int msg_type = getMsgType(i);
            if (msg_type == -1) {
              return -1;
            }
              
            switch (msg_type) {
                case LOC_REQUEST:
                handle_loc_request(i, msg_len);
                break;
                      
                case REGISTER:
                handle_register_request(i, msg_len);
                break;
                
                case TERMINATE:
                handle_terminate_request(i);
                all_close = true;
                break;
            }
          }
        }
          
      }
    }

    
    return 0;
}
