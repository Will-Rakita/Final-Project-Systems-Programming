#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "helpers.h"

struct House* house_init() {
    struct House* house = calloc(1, sizeof(struct House));
    if (!house) return NULL;

    // Initialize case file
    casefile_init(&house->case_file);

    // Initialize hunter collection
    house->hunter_capacity = 4;
    house->hunters = malloc(sizeof(struct Hunter*) * house->hunter_capacity);
    if (!house->hunters) {
        free(house);
        return NULL;
    }
    house->hunter_count = 0;
    house->room_count = 0;
    house->ghost = NULL;
    house->starting_room = NULL;

    return house;
}

void house_cleanup(struct House* house) {
    if (!house) return;

    // Cleanup ghost
    if (house->ghost) {
        ghost_cleanup(house->ghost);
    }

    // Cleanup hunters
    for (int i = 0; i < house->hunter_count; i++) {
        if (house->hunters[i]) {
            hunter_cleanup(house->hunters[i]);
        }
    }
    free(house->hunters);

    // Cleanup case file
    casefile_cleanup(&house->case_file);

    // Cleanup rooms
    for (int i = 0; i < house->room_count; i++) {
        room_cleanup(&house->rooms[i]);
    }

    free(house);
}

void hunter_collection_append(struct House* house, struct Hunter* hunter) {
    if (!house || !hunter) return;
    // Grow array if needed
    if (house->hunter_count >= house->hunter_capacity) {
        int new_capacity = house->hunter_capacity * 2;
        struct Hunter** new_hunters = realloc(house->hunters, 
                                            sizeof(struct Hunter*) * new_capacity);
        if (!new_hunters) return;
        house->hunters = new_hunters;
        house->hunter_capacity = new_capacity;
    }
    house->hunters[house->hunter_count++] = hunter;
}