#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "game/not.civ.6.h"
#include "net/server.h"
#include "net/message.h"

void conn_callback(nc6_client_t* client)
{
	// Print IP address of connected client
	char client_host_str[INET6_ADDRSTRLEN];

	struct sockaddr_in* addr = (struct sockaddr_in*)&(client->_addr_info);
	inet_ntop(addr->sin_family, &(addr->sin_addr), client_host_str, INET6_ADDRSTRLEN);

	printf("[SERVER] New client connected from: %s\n", client_host_str);
}

void disc_callback(nc6_client_t* client)
{
	char client_host_str[INET6_ADDRSTRLEN];

	struct sockaddr_in* addr = (struct sockaddr_in*)&(client->_addr_info);
	inet_ntop(addr->sin_family, &(addr->sin_addr), client_host_str, INET6_ADDRSTRLEN);

	printf("[SERVER] Client has disconnected from: %s\n", client_host_str);
}

void msg_callback(nc6_msg_t* msg)
{
    switch(msg->header.message_type)
    {
        case NC6_MSG_COMMSCHECK:
        {
            break;
        }
    }
}

int main() 
{
    // Start game
    printf("[GAME] Creating game...\n");
    nc6_gameopts_t nc6_options;
    nc6_options.max_connected_commanders = 8;
    nc6_options.board_height = 1000;
    nc6_options.board_width = 1000;

    printf("[GAME] Max connected commanders: %u\n", nc6_options.max_connected_commanders);
    printf("[GAME] Board dimensions: (%llu, %llu)\n", nc6_options.board_height, nc6_options.board_width);

    // Start server
    printf("[SERVER] Creating server...\n");
    nc6_serveropts_t nc6_serveropts;
    nc6_serveropts.port = 6969;
    nc6_serveropts.max_concurrent_connections = 255;
    nc6_serveropts.conn_callback = conn_callback;
    nc6_serveropts.disc_callback = disc_callback;
    nc6_serveropts.msg_callback = msg_callback;

    printf("[SERVER] Server port: %u\n", nc6_serveropts.port);

    nc6_server_t* nc6_server;

    if(nc6_server_create(&nc6_serveropts, &nc6_server) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to create server!");
        exit(1);
    }

    printf("[SERVER] Server created\n");

    // Start server
    printf("[SERVER] Starting server...\n");

    if(nc6_server_start(nc6_server) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to start server!");
        exit(1);
    }

    printf("[SERVER] Server started");
	return 0;
}
