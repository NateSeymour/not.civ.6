#ifndef _SERVER_H_
#define _SERVER_H_ 

#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>

// Server status
#define SERVER_RUNNING 0
#define SERVER_STALLED 1

// Server magic
#define PROTOCOL_MAGIC "BOATSANDHOES"
#define SERVER_UNAME "SERVER"
#define SERVER_SECRET "SERVER"

// pthread routine
typedef void*(*pthread_routine_t)(void*);

// Net headers
typedef struct __attribute__((__packed__)) {
    char magic[12];
    uint16_t muid;
    uint8_t message_type;
    uint16_t message_length;
    
    char username[20];
    char secret[20];
} nc6_header_t;

// Client struct
typedef struct {
    int _sockfd;
    struct sockaddr _addr_info;
} nc6_client_t;

// Messages
typedef struct {
    nc6_client_t* client;
    nc6_header_t header;
    char* payload;
} nc6_msg_t;

// Callbacks
typedef void(*nc6_connection_callback_t)(nc6_client_t*);
typedef void(*nc6_message_callback_t)(nc6_msg_t*);
typedef void(*nc6_disconnect_callback_t)(nc6_client_t*);

// Server struct
typedef struct {
    uint16_t port;
    uint8_t max_concurrent_connections;
    int status;

    int sockfd;
    struct addrinfo* servinfo;

    nc6_connection_callback_t conn_callback;
    nc6_message_callback_t msg_callback;
    nc6_disconnect_callback_t disc_callback;

    pthread_t connection_thread;
    pthread_mutex_t connection_pool_m;
    nc6_client_t* connection_pool;
    struct pollfd* connection_pool_pollfds;
    size_t connection_pool_size;
    size_t connected_client_count;

    pthread_t communication_thread;
} nc6_server_t;

// Options struct
typedef struct {
    uint16_t port;
    uint8_t max_concurrent_connections;

    nc6_connection_callback_t conn_callback;
    nc6_message_callback_t msg_callback;
    nc6_disconnect_callback_t disc_callback;
} nc6_serveropts_t;

// Methods
int nc6_server_create(nc6_serveropts_t* options, nc6_server_t** out);
int nc6_server_start(nc6_server_t* nc6_server);
void nc6_server_destroy(nc6_server_t* game_server);

void nc6_msg_reply(nc6_msg_t* msg, uint8_t type, char* payload, uint16_t payload_len);
void nc6_msg_send(nc6_client_t* client, uint16_t muid, uint8_t type, char* payload, uint16_t payload_len);

void nc6_client_kick(nc6_client_t* client);

#endif // _SERVER_H_
