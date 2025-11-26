#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "defs.h"
#include "helpers.h"

void roomstack_init(struct RoomStack* stack) {
    if (!stack) return;
    stack->head = NULL;
}

void roomstack_push(struct RoomStack* stack, struct Room* room) {
    if (!stack || !room) return;
    struct RoomNode* new_node = malloc(sizeof(struct RoomNode));
    if (!new_node) return;
    new_node->room = room;
    new_node->next = stack->head;
    stack->head = new_node;
}

struct Room* roomstack_pop(struct RoomStack* stack) {
    if (!stack || !stack->head) return NULL;
    struct RoomNode* node = stack->head;
    struct Room* room = node->room;
    stack->head = node->next;
    free(node);
    return room;
}

void roomstack_clear(struct RoomStack* stack) {
    if (!stack) return;
    while (stack->head) {
        roomstack_pop(stack);
    }
}

bool roomstack_is_empty(struct RoomStack* stack) {
    return !stack || !stack->head;
}

struct Hunter* hunter_init(const char* name, int id, struct House* house) {
    if (!house || !house->starting_room) return NULL;
    struct Hunter* hunter = malloc(sizeof(struct Hunter));
    if (!hunter) return NULL;

    strncpy(hunter->name, name, MAX_HUNTER_NAME - 1);
    hunter->name[MAX_HUNTER_NAME - 1] = '\0';
    hunter->id = id;
    hunter->current_room = NULL; // so room_add_hunter sets this
    hunter->house = house;
    hunter->case_file = &house->case_file;
    hunter->boredom = 0;
    hunter->fear = 0;
    hunter->is_running = true;
    hunter->exit_reason = LR_BORED;
    hunter->return_to_van = false;

    roomstack_init(&hunter->path);

    const enum EvidenceType* evidence_types = NULL;
    int evidence_count = get_all_evidence_types(&evidence_types);
    if (evidence_count > 0) {
        hunter->device = evidence_types[rand_int_threadsafe(0, evidence_count)];
    } else {
        hunter->device = EV_EMF;
    }

    if (!room_add_hunter(house->starting_room, hunter)) {
        free(hunter);
        return NULL;
    }
    return hunter;
}

void hunter_cleanup(struct Hunter* hunter) {
    if (!hunter) return;
    roomstack_clear(&hunter->path);
    free(hunter);
}

void hunter_update_stats(struct Hunter* hunter) {
    if (!hunter || !hunter->current_room) return;
    if (hunter->current_room->ghost) {
        hunter->boredom = 0;
        hunter->fear++;
    } else {
        hunter->boredom++;
        hunter->fear = 0;
    }
}

bool hunter_check_exit_conditions(struct Hunter* hunter) {
    if (!hunter) return false;
    if (hunter->boredom > ENTITY_BOREDOM_MAX) {
        hunter->exit_reason = LR_BORED;
        hunter->is_running = false;
        log_exit(hunter->id, hunter->boredom, hunter->fear,
                 hunter->current_room->name, hunter->device, LR_BORED);
        return true;
    }
    if (hunter->fear > HUNTER_FEAR_MAX) {
        hunter->exit_reason = LR_AFRAID;
        hunter->is_running = false;
        log_exit(hunter->id, hunter->boredom, hunter->fear,
                 hunter->current_room->name, hunter->device, LR_AFRAID);
        return true;
    }
    return false;
}

void hunter_van_check(struct Hunter* hunter) {
    if (!hunter || !hunter->house) return;
    if (hunter->current_room == hunter->house->starting_room) {
        roomstack_clear(&hunter->path);
        hunter->return_to_van = false;
        log_return_to_van(hunter->id, hunter->boredom, hunter->fear,
                          hunter->current_room->name, hunter->device, false);
        if (casefile_is_solved(hunter->case_file)) {
            hunter->exit_reason = LR_EVIDENCE;
            hunter->is_running = false;
            log_exit(hunter->id, hunter->boredom, hunter->fear,
                     hunter->current_room->name, hunter->device, LR_EVIDENCE);
            return;
        }
        enum EvidenceType old_device = hunter->device;
        const enum EvidenceType* evidence_types = NULL;
        int evidence_count = get_all_evidence_types(&evidence_types);
        if (evidence_count > 0) {
            enum EvidenceType new_device;
            do {
                new_device = evidence_types[rand_int_threadsafe(0, evidence_count)];
            } while (new_device == old_device && evidence_count > 1);
            hunter->device = new_device;
            log_swap(hunter->id, hunter->boredom, hunter->fear, old_device, new_device);
        }
    }
}

void hunter_gather_evidence(struct Hunter* hunter) {
    if (!hunter || !hunter->current_room) return;
    if (room_has_evidence(hunter->current_room, hunter->device)) {
        if (room_remove_evidence(hunter->current_room, hunter->device)) {
            casefile_add_evidence(hunter->case_file, hunter->device);
            log_evidence(hunter->id, hunter->boredom, hunter->fear,
                         hunter->current_room->name, hunter->device);
            if (!hunter->current_room->is_exit) {
                hunter->return_to_van = true;
                log_return_to_van(hunter->id, hunter->boredom, hunter->fear,
                                 hunter->current_room->name, hunter->device, true);
            }
        }
    } else {
        if (rand_int_threadsafe(0, 100) < 10) {
            hunter->return_to_van = true;
            log_return_to_van(hunter->id, hunter->boredom, hunter->fear,
                             hunter->current_room->name, hunter->device, true);
        }
    }
}

void hunter_move(struct Hunter* hunter) {
    if (!hunter || !hunter->current_room) return;
    struct Room* target_room = NULL;
    struct Room* prev_room = hunter->current_room;
    if (hunter->return_to_van) {
        target_room = roomstack_pop(&hunter->path);
        if (!target_room) {
            hunter->return_to_van = false;
            return;
        }
    } else {
        target_room = room_get_random_connection(hunter->current_room);
        if (!target_room) return;
    }
    if (room_move_entity(hunter->current_room, target_room, hunter)) {
        log_move(hunter->id, hunter->boredom, hunter->fear,
                 prev_room->name, target_room->name, hunter->device);

        if (!hunter->return_to_van) {
            roomstack_push(&hunter->path, prev_room);
        }
    }
}

void* hunter_thread(void* arg) {
    struct Hunter* hunter = (struct Hunter*)arg;
    if (!hunter) return NULL;

    printf("Hunter %d thread started\n", hunter->id);

    while (hunter->is_running) {
        hunter_update_stats(hunter);
        if (hunter_check_exit_conditions(hunter)) {
            break;
        }
        hunter_van_check(hunter);
        if (!hunter->is_running) {
            break;
        }
        hunter_gather_evidence(hunter);
        hunter_move(hunter);
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000L };
        nanosleep(&ts, NULL);
    }

    if (hunter->current_room) {
        room_remove_hunter(hunter->current_room, hunter);
    }
    printf("Hunter %d thread exiting\n", hunter->id);
    return NULL;
}
