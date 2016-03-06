Group Members: Andrew Jenkins(ID: a5jenkin, student ID: 20438987), Dingzhong Chen(ID: dzchen, student ID: 20494213)


Instructions

Compiler: Any compiler that supports the C++98 standard
Make: The version on the student server

Undergrad machines:
binder is built and tested on: ubuntu1204-002
server is built and tested on: ubuntu1204-004
client is built and tested on: ubuntu1204-006


How to run:
Note: If you are using client1.cc and server.cc in the PartialTestCodes that are providing on the course website, you can just compile to get binder, client, server and librpc.a executable by: make 

1.compile to get binder executable: make binder
2.compile to get librpc.a executable: make lirpc.a
3.compile client: g++ -Wall -MMD -g -pthread -L. client.o -lrpc -o client
4.compile server: g++ -Wall -MMD -g -pthread -L. server_functions.o server_function_skels.o server.o librpc.a -lrpc -o server
5.run binder: ./binder
6.Manually set the BINDER_ADDRESS and BINDER_PORT environment variables on the client/server machine
7.run server: ./server
8.run client: ./client
9.clean: make clean
