#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "net/server.h"
#include "game/options.h"

void conn_callback(nc6_client_t* client)
{
	// Print IP address of connected client
	char client_host_str[INET6_ADDRSTRLEN];

	struct sockaddr_in* addr = (struct sockaddr_in*)&(client->_addr_info);
	inet_ntop(addr->sin_family, &(addr->sin_addr), client_host_str, INET6_ADDRSTRLEN);

	printf("New client connected from: %s\n", client_host_str);
}

void disc_callback(nc6_client_t* client)
{
	char client_host_str[INET6_ADDRSTRLEN];

	struct sockaddr_in* addr = (struct sockaddr_in*)&(client->_addr_info);
	inet_ntop(addr->sin_family, &(addr->sin_addr), client_host_str, INET6_ADDRSTRLEN);

	printf("Client has disconnected from: %s\n", client_host_str);
}

void msg_callback(nc6_msg_t* msg)
{

}

int main() 
{
    // Create game
    nc6_options_t nc6_options;
    nc6_options.max_connected_commanders = 8;
    nc6_options.board_height = 1000;
    nc6_options.board_width = 1000;

    // Create server
    nc6_serveropts_t nc6_serveropts;
    nc6_serveropts.port = 6969;
    nc6_serveropts.max_concurrent_connections = 255;
    nc6_serveropts.conn_callback = conn_callback;
    nc6_serveropts.disc_callback = disc_callback;
    nc6_serveropts.msg_callback = msg_callback;

    nc6_server_t* nc6_server;

    if(nc6_server_create(&nc6_serveropts, &nc6_server) != 0)
    {
        fprintf(stderr, "Failed to create server!");
        exit(1);
    }



	return 0;
}
