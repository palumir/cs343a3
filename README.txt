Group Members: Andrew Jenkins(ID: a5jenkin, student ID: 20438987), Dingzhong Chen(ID: dzchen, student ID: 20494213)


Instructions

Compiler: Any compiler that supports the C++98 standard
Make: The version on the student server

Undergrad machines:
binder is built and tested on: ubuntu1204-002
server is built and tested on: ubuntu1204-004
client is built and tested on: ubuntu1204-006


How to run:
Note: If you are using client1.c and server.c in the PartialTestCodes that are providing on the course website, you can just compile to get binder, client, server and librpc.a executable by: make 

1.compile to get binder executable: make binder
2.compile to get librpc.a executable: make librpc.a

3.get object file of client: cc    -c -o client1.o client1.c 
4.compile client: g++ -Wall -MMD -g -pthread -L. client1.o -lrpc -o client

5.get object file of server_functions: cc    -c -o server_functions.o server_functions.c
6.get object file of server_function_skels: cc    -c -o server_function_skels.o server_function_skels.c
7.get object file of server: cc    -c -o server.o server.c
8.compile client: g++ -Wall -MMD -g -pthread -L. server_functions.o server_function_skels.o server.o -lrpc -o server

9.run binder: ./binder
10.Manually set the BINDER_ADDRESS and BINDER_PORT environment variables on the client/server machine
11.run server: ./server
12.run client: ./client
13.clean: make clean
