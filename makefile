CFLAGS = -c -g -Wall -Wextra -Werror -Wshadow -Wdouble-promotion -Wformat=2 -Wformat-overflow \
				 -Wformat-truncation -fno-common -Wconversion
S = src/server_code/
CL = src/client_code/
CO = src/common/
CLTEST = client_test_directory
STEST = server_test_directory

all: server client

server: server.o network_node.o packet.o resource.o serverPacket.o
	gcc server.o network_node.o packet.o resource.o serverPacket.o -o server
	mv server server_test_directory

client: client.o network_node.o packet.o tcp.o clientPacket.o
	gcc client.o network_node.o packet.o tcp.o clientPacket.o -o client
	cp client client_test_directory01
	mv client client_test_directory

client.o: $(CL)client.c $(CL)client.h
	gcc $(CFLAGS) $(CL)client.c

server.o: $(S)server.c $(S)server.h 
	gcc $(CFLAGS) $(S)server.c

network_node.o: $(CO)network_node.c $(CO)network_node.h
	gcc $(CFLAGS) $(CO)network_node.c

packet.o: $(CO)packet.c $(CO)packet.h
	gcc $(CFLAGS) $(CO)packet.c

resource.o: $(S)resource.c $(S)resource.h
	gcc $(CFLAGS) $(S)resource.c

tcp.o: $(CO)tcp.c $(CO)tcp.h
	gcc $(CFLAGS) $(CO)tcp.c

clientPacket.o: $(CL)clientPacket.c $(CL)clientPacket.h
	gcc $(CFLAGS) $(CL)clientPacket.c

serverPacket.o: $(S)serverPacket.c $(S)serverPacket.h
	gcc $(CFLAGS) $(S)serverPacket.c

clean:
	rm client_test_directory01/client
	rm $(CLTEST)/client	# Only removing the executable
	rm $(STEST)/server
	rm *.o

# request test01.txt
