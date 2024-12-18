CFLAGS = -c -g -Wall -Wextra -Werror -Wshadow -Wdouble-promotion -Wformat=2 -Wformat-overflow \
				 -Wformat-truncation -fno-common -Wconversion
S = src/server_code/
CL = src/client_code/
CO = src/common/
CLTEST = client_test_directory
STEST = server_test_directory

all: server client

server: server.o network_node.o packet.o resource.o
	gcc server.o network_node.o packet.o resource.o -o server
	# mkdir -p server_test_directory
	mv server server_test_directory

client: client.o network_node.o packet.o
	gcc client.o network_node.o packet.o -o client
	# mkdir -p client_test_directory
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

clean:
	#rm -rf $(CLTEST)
	rm $(CLTEST)/client	# Only removing the executable
	#rm -rf $(STEST)
	rm $(STEST)/server
	rm *.o
