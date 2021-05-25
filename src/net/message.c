#include <stdlib.h>
#include <string.h>

#include "server.h"

void nc6_msg_reply(nc6_msg_t* msg, uint8_t type, char* payload, uint16_t payload_len)
{
    nc6_msg_send(msg->client, msg->header.muid, type, payload, payload_len);
}

void nc6_msg_send(nc6_client_t* client, uint16_t muid, uint8_t type, char* payload, uint16_t payload_len)
{
    const size_t header_size = sizeof(nc6_header_t);
    nc6_header_t header;

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
    memcpy(&(header.magic), PROTOCOL_MAGIC, strlen(PROTOCOL_MAGIC));
    memcpy(&(header.username), SERVER_UNAME, strlen(SERVER_UNAME));
    memcpy(&(header.secret), SERVER_SECRET, strlen(SERVER_SECRET));

    header.muid = muid;
    header.message_type = type;

    // Create message
    memcpy(data, &header, header_size);
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