#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>

#include "server.h"

/// Create a new nc6 server instance
/// \param options Options for the server (callbacks, port, etc)
/// \param out Pointer to the new server structure
/// \return 0 on success. 1 on failure.
int nc6_server_create(nc6_serveropts_t* options, nc6_server_t** out)
{
	nc6_server_t* nc6_server = (nc6_server_t*)malloc(sizeof(nc6_server_t));

    nc6_server->status = SERVER_STALLED;
    nc6_server->port = options->port;
    nc6_server->max_concurrent_connections = options->max_concurrent_connections;

    nc6_server->conn_callback = options->conn_callback;
    nc6_server->disc_callback = options->disc_callback;
    nc6_server->msg_callback = options->msg_callback;

    nc6_server->connection_pool = NULL;

	struct addrinfo hints;

	// Start a server on ipv4/tcp
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// Convert port number to a string
	char port_str[10];
	snprintf(port_str, sizeof(port_str), "%u", nc6_server->port);

	int res_addrinfo = getaddrinfo(NULL, port_str, &hints, &nc6_server->servinfo);
	if(res_addrinfo != 0)
	{
        free(nc6_server);
		return 1;
	}

    nc6_server->sockfd = socket(nc6_server->servinfo->ai_family, nc6_server->servinfo->ai_socktype, nc6_server->servinfo->ai_protocol);
	if(nc6_server->sockfd == -1)
	{
        free(nc6_server);
		return 1;
	}

	// Bind the socket to our address
	int res_bind = bind(nc6_server->sockfd, nc6_server->servinfo->ai_addr, nc6_server->servinfo->ai_addrlen);
	if(res_bind != 0) 
	{
        nc6_server_destroy(nc6_server);
		return 1;
	}

	*out = nc6_server;
    return 0;
}

_Noreturn void* nc6_server_comms_routine(nc6_server_t* nc6_server)
{
	const size_t header_size = sizeof(nc6_header_t);
	nc6_header_t header_buffer;

	const size_t small_msg_size = 1024;
	char small_msg_buffer[small_msg_size];

	while(1)
	{
		// Poll for incoming data, scanning accross all connections.
		int count = poll(nc6_server->connection_pool_pollfds, nc6_server->connected_client_count, 100);
		if(count == 0) continue;
		
		for(int i = 0; i < nc6_server->connected_client_count; i++)
		{
			struct pollfd* pollfd = &(nc6_server->connection_pool_pollfds[i]);

			// Client has sent data
			if(pollfd->revents & POLLIN)
			{
				size_t header_received_bytes = recv(pollfd->fd, &header_buffer, header_size, MSG_WAITALL);

				// If bytes == 0, then the client has disconnected
				if(header_received_bytes == 0)
				{
					// Close connection
					close(pollfd->fd);

					// Call disconnect callback
					if(nc6_server->disc_callback != NULL)
					{
						nc6_server->disc_callback(&(nc6_server->connection_pool[i]));
					}
	
					// Remove connection
					pthread_mutex_lock(&(nc6_server->connection_pool_m));

					memset(&(nc6_server->connection_pool[i]), 0, sizeof(nc6_client_t));
					memset(&(nc6_server->connection_pool_pollfds[i]), 0, sizeof(struct pollfd));
	
					nc6_server->connected_client_count--;
		
					pthread_mutex_unlock(&(nc6_server->connection_pool_m));
					continue;
				}

				// Check the magic value
				if(strncmp(header_buffer.magic, PROTOCOL_MAGIC, strlen(PROTOCOL_MAGIC)) != 0)
				{
					// TODO: send error message to client
					continue;
				}
				
				// Set the message length to host byte order
				header_buffer.message_length = ntohs(header_buffer.message_length) - sizeof(nc6_header_t);

				// If the data fits into the place allocated on the stack, put it there. 
				// Otherwise space will be dynamically allocated to fit it.
				char* data;
				unsigned short data_size = header_buffer.message_length;
				if(header_buffer.message_length > small_msg_size)
				{
					data = malloc(data_size);
				} 
				else
				{
					data = small_msg_buffer;
					memset(data, 0, small_msg_size);
				}

				// Read in the rest of the message to the buffer
				size_t payload_received_bytes = recv(pollfd->fd, data, header_buffer.message_length, 0);

				// Call the callback if available
				if(nc6_server->msg_callback != NULL)
				{
					nc6_msg_t message;
					message.client = &(nc6_server->connection_pool[i]);
					message.header = header_buffer;
					message.payload = data;

					nc6_server->msg_callback(&message);
				}

				if(data != small_msg_buffer)
				{
					free(data);
				}
			}
		}
	}
}

_Noreturn void* nc6_server_connection_routine(nc6_server_t* nc6_server)
{
	listen(nc6_server->sockfd, 20);

	struct sockaddr incoming_addr;
	socklen_t addr_size = sizeof(incoming_addr);

	// Infinitely listen for and accept new clients
	while(1)
	{
		// Accept incoming connections
		int clientfd = accept(nc6_server->sockfd, &incoming_addr, &addr_size);

		// Check to see if we can handle the connection
		if(nc6_server->connected_client_count + 1 > nc6_server->max_concurrent_connections)
        {
		    // TODO: send client close notification message
		    close(clientfd);
		    continue;
        }

		// Lock mutex
		pthread_mutex_lock(&(nc6_server->connection_pool_m));

		// Find suitable index
		int new_client_index;
		for(new_client_index = 0; new_client_index < nc6_server->connection_pool_size; new_client_index++)
        {
		    if(nc6_server->connection_pool[new_client_index]._sockfd == 0) break;
        }

		// Client
		nc6_client_t* client = &(nc6_server->connection_pool[new_client_index]);
		client->_sockfd = clientfd;
		
		memcpy(&(client->_addr_info), &incoming_addr, addr_size);
		
		// Pollfd entry
		struct pollfd* pollfd = &(nc6_server->connection_pool_pollfds[new_client_index]);
        pollfd->fd = clientfd;
        pollfd->events = POLLIN | POLLHUP;
        pollfd->revents = (short)0;

		// Increment client count
		nc6_server->connected_client_count++;

		// Call connection callback (if exists)
		if(nc6_server->conn_callback != NULL)
		{
			nc6_server->conn_callback(client);
		}

		// Unlock mutex
		pthread_mutex_unlock(&(nc6_server->connection_pool_m));
	}
}

int nc6_server_start(nc6_server_t* nc6_server)
{
	// Exit if the server is already running
	if(nc6_server->status != SERVER_STALLED)
	{
		return 1;
	}

	// Initialize RAM for connection pool and pollfd's
	if(nc6_server->connection_pool != NULL)
	{
		free(nc6_server->connection_pool);
	}

    nc6_server->connected_client_count = 0;
    nc6_server->connection_pool_size = nc6_server->max_concurrent_connections;

    nc6_server->connection_pool = calloc(nc6_server->connection_pool_size, sizeof(nc6_client_t));
    nc6_server->connection_pool_pollfds = calloc(nc6_server->connection_pool_size, sizeof(struct pollfd));
	
	// Initialize connection routine with mutex for connection list
	int res_conmutex = pthread_mutex_init(&(nc6_server->connection_pool_m), NULL);
	if(res_conmutex != 0)
	{
		return 1;
	}

	int res_conthread = pthread_create(&(nc6_server->connection_thread), NULL, (pthread_routine_t) nc6_server_connection_routine, nc6_server);
	if(res_conthread != 0)
	{
		return 1;
	}

	// Initialize communication routine
	int res_comthread = pthread_create(&(nc6_server->communication_thread), NULL, (pthread_routine_t) nc6_server_comms_routine, nc6_server);
	if(res_comthread != 0) 
	{
		return 1;
	}

	// Set server status
	nc6_server->status = SERVER_RUNNING;

	return 0;
}

void nc6_server_destroy(nc6_server_t* game_server)
{
	// Set server status
	game_server->status = SERVER_STALLED; 

	// Cancel threads
	pthread_cancel(game_server->connection_thread);
	pthread_cancel(game_server->communication_thread);

	pthread_join(game_server->connection_thread, NULL);
	pthread_join(game_server->communication_thread, NULL);

	// Close client connections
	for(int i = 0; i < game_server->connected_client_count; i++)
	{
		close(game_server->connection_pool[i]._sockfd);
	}

	// Close server socket
	close(game_server->sockfd);

	// Destroy mutexes
	pthread_mutex_destroy(&(game_server->connection_pool_m));

	// Free server info
	freeaddrinfo(game_server->servinfo);

	// Free server structure
	if(game_server->connection_pool != NULL)
	{
		free(game_server->connection_pool);
	}
	free(game_server);
}
