#ifndef NOT_CIV_6_OPTIONS_H
#define NOT_CIV_6_OPTIONS_H

#include <stdint.h>

typedef struct {
    uint8_t max_connected_commanders;

    uint64_t board_width;
    uint64_t board_height;
} nc6_options_t;

#endif //NOT_CIV_6_OPTIONS_H
