#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <lua.h>
#include <lualib.h>

#include "main.h"
#include "net/server.h"
#include "game/message.h"

void conn_callback(game_client_t* client) 
{
	// Print IP address of connected client
	char client_host_str[INET6_ADDRSTRLEN];

	struct sockaddr_in* addr = (struct sockaddr_in*)&(client->_addr_info);
	inet_ntop(addr->sin_family, &(addr->sin_addr), client_host_str, INET6_ADDRSTRLEN);

	printf("New client connected from: %s\n", client_host_str);
}

void disc_callback(game_client_t* client)
{
	char client_host_str[INET6_ADDRSTRLEN];

	struct sockaddr_in* addr = (struct sockaddr_in*)&(client->_addr_info);
	inet_ntop(addr->sin_family, &(addr->sin_addr), client_host_str, INET6_ADDRSTRLEN);

	printf("Client has disconnected from: %s\n", client_host_str);
}

void data_callback(game_client_t* client, sg_message_t* msg)
{
	char client_host_str[INET6_ADDRSTRLEN];

	struct sockaddr_in* addr = (struct sockaddr_in*)&(client->_addr_info);
	inet_ntop(addr->sin_family, &(addr->sin_addr), client_host_str, INET6_ADDRSTRLEN);

	printf("Data (%s): %s\n", client_host_str, msg->payload);

	switch(msg->header.message_type)
	{
		case MSG_COMMSCHECK: 
		{
			reply_to_msg(client, msg, MSG_COMMSCHECK, "OK", 2);
			break;	
		}
	}
}

game_server_t* initialize_server()
{
	game_server_t* game_server;
	int res_server = create_game_server(6969, 
			conn_callback, 
			disc_callback, 
			data_callback, 
			&game_server);

	if(res_server != 0)
	{
		fprintf(stderr, "Error: Failed to create server!\n");
		exit(1);
	}
	
	return game_server;
}

int main() 
{
	printf("Initializing server...\n");
	game_server_t* game_server = initialize_server();

	printf("Starting server...\n");	
	int res_servstart = start_game_server(game_server); 
	if(res_servstart != 0)
	{
		fprintf(stderr, "Error: Failed to start server!\n");
		exit(1);
	}
	
	getchar();

	printf("Shutting down server...\n");
	destroy_game_server(game_server);

	return 0;
}
