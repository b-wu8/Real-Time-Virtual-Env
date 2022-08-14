include ../Makefile.general 

.PHONY: all clean distclean

top_srcdir=..

SERVER_PROGRAMS=oculus_server test_client
CFLAGS	   += `pkg-config --cflags 'libprotobuf-c >= 1.0.0'`
LDFLAGS	   += `pkg-config --libs 'libprotobuf-c >= 1.0.0'` 

all:$(SERVER_PROGRAMS)

oculus_server: oculus_server.o oculus_create_response.o oculus_handlers.o oculus_context.o oculus_utils.o oculus_protobuf.o oculus_priorityqueue.o oculus_server_api.o $(LIBSPINES_LIB_FILE)
		$(CC) $(LDFLAGS) -o $ oculus_server oculus_server.o oculus_create_response.o oculus_handlers.o oculus_context.o oculus_utils.o oculus_protobuf.o oculus_priorityqueue.o oculus_server_api.o game.pb-c.o $(LIBS) -lm

oculus_server.o: oculus_server.c ./protoc_generated/game.pb-c.c
		$(CC) $(CFLAGS) -g -c $^

test_client: test_client.o game.pb-c.o oculus_utils.o $(LIBSPINES_LIB_FILE)
		$(CC) $(LDFLAGS) -o $ test_client test_client.o game.pb-c.o oculus_utils.o $(LIBS) -lm

test_client.o: test_client.c ./protoc_generated/game.pb-c.c
		$(CC) $(CFLAGS) -g -c $^

clean:
	rm -f *.o
	rm -f $(SERVER_PROGRAMS)

distclean: clean
	rm -f *~
	rm -f core*
	rm -f Makefile
