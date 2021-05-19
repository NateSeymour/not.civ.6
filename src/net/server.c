#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>

#include "server.h"

int create_game_server(unsigned short port, 
		client_connection_callback_t conn_callback,
		client_disconnect_callback_t disc_callback,
		client_data_callback_t data_callback,
		game_server_t** out)
{
	game_server_t* game_server = (game_server_t*)malloc(sizeof(game_server_t));

	game_server->status = SERVER_STALLED;
	game_server->port = port;

	game_server->_conn_callback = conn_callback;
	game_server->_disc_callback = disc_callback;
	game_server->_data_callback = data_callback;

	game_server->_connection_pool = NULL;

	struct addrinfo hints;

	// Start a server on ipv4/tcp
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// Convert port number to a string
	char port_str[10];
	snprintf(port_str, sizeof(port_str), "%u", port);

	int res_addrinfo = getaddrinfo(NULL, port_str, &hints, &game_server->_servinfo);
	if(res_addrinfo != 0)
	{
		return 1;
	}

	game_server->_sockfd = socket(game_server->_servinfo->ai_family, game_server->_servinfo->ai_socktype, game_server->_servinfo->ai_protocol);
	if(game_server->_sockfd == -1)
	{
		return 1;
	}

	// Bind the socket to our address
	int res_bind = bind(game_server->_sockfd, game_server->_servinfo->ai_addr, game_server->_servinfo->ai_addrlen);
	if(res_bind != 0) 
	{
		return 1;
	}

	*out = game_server;
    return 0;
}

void send_msg_to_client(game_client_t* client, unsigned char type, char* payload, size_t payload_len)
{
	const size_t header_size = sizeof(sg_header_t);
	sg_header_t header_buffer;

	const size_t small_msg_size = 1024;
	char small_msg_buffer[small_msg_size];

	// Use the stack buffer to compose message if it's big enough, otherwise 
	// allocate RAM dynamically to put it in
	char* data;
	size_t message_size = header_size + payload_len;
	if(message_size > small_msg_size)
	{
		data = malloc(message_size);	
	}
	else 
	{
		data = small_msg_buffer;
		memset(data, 0, small_msg_size);
	}

	// Fill header values
	memcpy(&(header_buffer.magic), PROTOCOL_MAGIC, strlen(PROTOCOL_MAGIC));
	memcpy(&(header_buffer.username), SERVER_UNAME, strlen(SERVER_UNAME));
	memcpy(&(header_buffer.secret), SERVER_SECRET, strlen(SERVER_SECRET));

	header_buffer.message_type = type;

	// Create message
	memcpy(data, &header_buffer, header_size);
	memcpy(data + header_size, payload, payload_len);

	// Send message
	// TODO: Make sure that the whole message has been sent
	send(client->_sockfd, data, message_size, 0);

	// Free RAM if it was dynamically allocated
	if(data != small_msg_buffer) 
	{
		free(data);
	}		
}

void* _server_communication_routine(game_server_t* game_server)
{
	const size_t header_size = sizeof(sg_header_t);
	sg_header_t header_buffer;

	const size_t small_msg_size = 1024;
	char small_msg_buffer[small_msg_size];

	while(1)
	{
		// Poll for incoming data, scanning accross all connections.
		int count = poll(game_server->_connection_pool_pollfds, game_server->_connected_client_count, 100);
		if(count == 0) continue;
		
		for(int i = 0; i < game_server->_connected_client_count; i++)
		{
			struct pollfd* pollfd = &(game_server->_connection_pool_pollfds[i]);

			// Client has sent data
			if(pollfd->revents & POLLIN)
			{
				ssize_t header_received_bytes = recv(pollfd->fd, &header_buffer, header_size, MSG_WAITALL);	

				// If bytes == 0, then the client has disconnected
				if(header_received_bytes == 0)
				{
					// Close connection
					close(pollfd->fd);

					// Call disconnect callback
					if(game_server->_disc_callback != NULL)
					{
						game_server->_disc_callback(&(game_server->_connection_pool[i]));
					}
	
					// Remove connection
					pthread_mutex_lock(&(game_server->_connection_pool_m));
	
					game_server->_connection_pool[i] = game_server->_connection_pool[game_server->_connected_client_count];
					game_server->_connection_pool_pollfds[i] = game_server->_connection_pool_pollfds[game_server->_connected_client_count];
	
					game_server->_connected_client_count--;
		
					pthread_mutex_unlock(&(game_server->_connection_pool_m));
					continue;
				}

				// Check the magic value
				if(strncmp(header_buffer.magic, PROTOCOL_MAGIC, strlen(PROTOCOL_MAGIC)) != 0)
				{
					// TODO: send error message to client
					continue;
				}
				
				// Set the message length to host byte order
				header_buffer.message_length = ntohs(header_buffer.message_length) - sizeof(sg_header_t);

				// If the data fits into the place allocated on the stack, put it there. 
				// Otherwise space will be dynamically allocated to fit it.
				char* data;
				short data_size = header_buffer.message_length;
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
				ssize_t payload_received_bytes = recv(pollfd->fd, data, header_buffer.message_length, 0);	

				// Call the callback if available
				if(game_server->_data_callback != NULL)
				{
					sg_message_t message;
					message.header = header_buffer;
					message.payload = data;

					game_server->_data_callback(&(game_server->_connection_pool[i]), &message);
				}

				if(data != small_msg_buffer)
				{
					free(data);
				}
			}
		}
	}
}

void* _server_connection_routine(game_server_t* game_server)
{
	listen(game_server->_sockfd, 20);

	struct sockaddr incoming_addr;
	socklen_t addr_size = sizeof(incoming_addr);

	// Infinitely listen for and accept new clients
	while(1)
	{
		// Accept incoming connections
		int clientfd = accept(game_server->_sockfd, &incoming_addr, &addr_size);

		// Lock mutex
		pthread_mutex_lock(&(game_server->_connection_pool_m));
		
		// Reallocate connection pool to handle next connection (if not big enough)
		if(game_server->_connected_client_count + 1 > game_server->_connection_pool_size)
		{
			game_server->_connection_pool_size += 10;
			game_server->_connection_pool = realloc(game_server->_connection_pool, sizeof(game_client_t) * game_server->_connection_pool_size);
			game_server->_connection_pool_pollfds = realloc(game_server->_connection_pool_pollfds, sizeof(struct pollfd) * game_server->_connection_pool_size);
		}

		// Client
		game_client_t* client = &(game_server->_connection_pool[game_server->_connected_client_count]);
		client->_sockfd = clientfd;
		
		memcpy(&(client->_addr_info), &incoming_addr, addr_size);
		
		// Pollfd entry
		struct pollfd* pollfd = &(game_server->_connection_pool_pollfds[game_server->_connected_client_count]);
		pollfd->fd = clientfd;
		pollfd->events = POLLIN | POLLHUP;
		pollfd->revents = (short)0;

		// Increment client count
		game_server->_connected_client_count++;

		// Call connection callback (if exists)
		if(game_server->_conn_callback != NULL)
		{
			game_server->_conn_callback(client);
		}

		// Unlock mutex
		pthread_mutex_unlock(&(game_server->_connection_pool_m));
	}
}

int start_game_server(game_server_t* game_server)
{
	// Exit if the server is already running
	if(game_server->status != SERVER_STALLED)
	{
		return 1;
	}

	// Initialize RAM for connection pool and pollfd's
	if(game_server->_connection_pool != NULL)
	{
		free(game_server->_connection_pool);
	}

	game_server->_connected_client_count = 0;
	game_server->_connection_pool_size = 20;
	game_server->_connection_pool = calloc(game_server->_connection_pool_size, sizeof(game_client_t));

	game_server->_connection_pool_pollfds = calloc(game_server->_connection_pool_size, sizeof(struct pollfd));
	
	// Initialize connection routine with mutex for connection list
	int res_conmutex = pthread_mutex_init(&(game_server->_connection_pool_m), NULL);
	if(res_conmutex != 0)
	{
		return 1;
	}

	int res_conthread = pthread_create(&(game_server->_connection_thread), NULL, (pthread_routine_t)_server_connection_routine, game_server);
	if(res_conthread != 0)
	{
		return 1;
	}

	// Initialize communication routine
	int res_comthread = pthread_create(&(game_server->_communication_thread), NULL, (pthread_routine_t)_server_communication_routine, game_server);
	if(res_comthread != 0) 
	{
		return 1;
	}

	// Set server status
	game_server->status = SERVER_RUNNING;

	return 0;
}

void destroy_game_server(game_server_t* game_server)
{
	// Set server status
	game_server->status = SERVER_STALLED; 

	// Cancel threads
	pthread_cancel(game_server->_connection_thread);
	pthread_cancel(game_server->_communication_thread);

	pthread_join(game_server->_connection_thread, NULL);
	pthread_join(game_server->_communication_thread, NULL);

	// Close client connections
	for(int i = 0; i < game_server->_connected_client_count; i++)
	{
		close(game_server->_connection_pool[i]._sockfd);
	}

	// Close server socket
	close(game_server->_sockfd);

	// Destroy mutexes
	pthread_mutex_destroy(&(game_server->_connection_pool_m));

	// Free server info
	freeaddrinfo(game_server->_servinfo); 

	// Free server structure
	if(game_server->_connection_pool != NULL)
	{
		free(game_server->_connection_pool);
	}
	free(game_server);
}
