#ifndef _SERVER_H_
#define _SERVER_H_ 

#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

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
    unsigned char message_type;
    short message_length;
    
    char username[20];
    char secret[20];
} sg_header_t;

// Client struct
typedef struct {
    int _sockfd;
    struct sockaddr _addr_info;
} game_client_t;

// Messages
typedef struct {
    sg_header_t header;
    char* payload;
} sg_message_t;

// Callbacks
typedef void(*client_connection_callback_t)(game_client_t*);
typedef void(*client_data_callback_t)(game_client_t*, sg_message_t*);
typedef void(*client_disconnect_callback_t)(game_client_t*);

// Server struct
typedef struct {
    short port;
    int status;

    int _sockfd;
    struct addrinfo* _servinfo;

    client_connection_callback_t _conn_callback;
    client_data_callback_t _data_callback;
    client_disconnect_callback_t _disc_callback;

    pthread_t _connection_thread;
    pthread_mutex_t _connection_pool_m;
    game_client_t* _connection_pool;
    struct pollfd* _connection_pool_pollfds;
    size_t _connection_pool_size;
    size_t _connected_client_count;

    pthread_t _communication_thread;
} game_server_t;

int create_game_server(unsigned short port, 
        client_connection_callback_t conn_callback,
        client_disconnect_callback_t disc_callback,
        client_data_callback_t data_callback,
        game_server_t** out);
int start_game_server(game_server_t* game_server);
void destroy_game_server(game_server_t* game_server);
void send_msg_to_client(game_client_t* client, unsigned char type, char* payload, size_t payload_len);

#endif // _SERVER_H_
