#ifndef _NOT_CIV_6_H_
#define _NOT_CIV_6_H_

#include <stdint.h>

#include "../net/server.h"

// Commander
typedef struct {
    char username[20];
    char secret[20];

    nc6_client_t* client;
} nc6_commander_t;

// Entity
typedef struct {
    nc6_commander_t* owner;

    uint32_t entity_type;

    uint64_t pos_x;
    uint64_t pos_y;

    uint8_t hp;
    uint8_t mp;
} nc6_entity_t;

typedef struct {
    char type_name[20];
    nc6_commander_t* owner;

    uint8_t is_public;

    uint8_t hp;
    uint8_t mp;
    uint8_t attack;
    uint8_t range;
    uint8_t defense;
    uint8_t crit_mod;
} nc6_entity_def_t;

// Game options
typedef struct {
    uint8_t max_connected_commanders;

    uint64_t board_width;
    uint64_t board_height;
} nc6_gameopts_t;

// Game
typedef struct {
    nc6_commander_t* commander_pool;
    uint8_t commander_pool_size;
    uint8_t commander_pool_count;


} nc6_game_t;

#endif // _NOT_CIV_6_H_
