CXX = g++
CXXFLAGS = -Wall -MMD -g -pthread
EXEC1 = binder
EXEC2 = client
EXEC3 = server
EXECUTABLE = client server binder librpc.a

all: ${EXECUTABLE}

librpc.a: rpc.o
		ar rc librpc.a rpc.o

${EXEC1}: binder.o
		${CXX} ${CXXFLAGS} binder.o -o ${EXEC1}

${EXEC2}: client1.o librpc.a
		${CXX} ${CXXFLAGS} -L. client1.o -lrpc -o ${EXEC2}

${EXEC3}: server_functions.o server_function_skels.o server.o librpc.a
		${CXX} ${CXXFLAGS} -L. server_functions.o server_function_skels.o server.o -lrpc -o ${EXEC3}


.PHONY: clean

clean:
		rm -f *.o ${EXECUTABLE}
