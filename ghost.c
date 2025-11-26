#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "defs.h"
#include "helpers.h"

struct Ghost* ghost_init(struct House* house) {
    if (!house || house->room_count == 0) return NULL;
    struct Ghost* ghost = malloc(sizeof(struct Ghost));
    if (!ghost) return NULL;

    ghost->id = DEFAULT_GHOST_ID;
    const enum GhostType* ghost_types = NULL;
    int ghost_count = get_all_ghost_types(&ghost_types);
    if (ghost_count > 0) {
        ghost->type = ghost_types[rand_int_threadsafe(0, ghost_count)];
    } else {
        ghost->type = GH_POLTERGEIST;
    }
    int start_room_index = rand_int_threadsafe(0, house->room_count);
    ghost->current_room = &house->rooms[start_room_index];
    ghost->boredom = 0;
    ghost->is_running = true;

    room_set_ghost(ghost->current_room, ghost);

    log_ghost_init(ghost->id, ghost->current_room->name, ghost->type);

    return ghost;
}

void ghost_cleanup(struct Ghost* ghost) {
    if (ghost && ghost->current_room) {
        room_remove_ghost(ghost->current_room);
    }
    free(ghost);
}

void ghost_update_stats(struct Ghost* ghost) {
    if (!ghost || !ghost->current_room) return;
    if (ghost->current_room->hunter_count > 0) {
        ghost->boredom = 0;
    } else {
        ghost->boredom++;
    }
}

bool ghost_check_exit_condition(struct Ghost* ghost) {
    if (!ghost) return false;
    if (ghost->boredom > ENTITY_BOREDOM_MAX) {
        ghost->is_running = false;
        log_ghost_exit(ghost->id, ghost->boredom, ghost->current_room->name);
        return true;
    }
    return false;
}

void ghost_take_action(struct Ghost* ghost) {
    if (!ghost || !ghost->current_room) return;
    int action = rand_int_threadsafe(0, 3);
    switch (action) {
        case 0:
            log_ghost_idle(ghost->id, ghost->boredom, ghost->current_room->name);
            break;
        case 1: {
            EvidenceByte ghost_evidence = (EvidenceByte)ghost->type;
            const enum EvidenceType* all_evidence = NULL;
            int evidence_count = get_all_evidence_types(&all_evidence);
            enum EvidenceType available_evidence[7];
            int available_count = 0;
            for (int i = 0; i < evidence_count; i++) {
                if (ghost_evidence & all_evidence[i]) {
                    available_evidence[available_count++] = all_evidence[i];
                }
            }
            if (available_count > 0) {
                enum EvidenceType chosen_evidence =
                    available_evidence[rand_int_threadsafe(0, available_count)];
                room_add_evidence(ghost->current_room, chosen_evidence);
                log_ghost_evidence(ghost->id, ghost->boredom,
                                   ghost->current_room->name, chosen_evidence);
            }
            break;
        }
        case 2:
            if (ghost->current_room->hunter_count == 0) {
                struct Room* target_room = room_get_random_connection(ghost->current_room);
                if (target_room && target_room != ghost->current_room) {
                    const char* from_room = ghost->current_room->name;
                    if (room_move_entity(ghost->current_room, target_room, ghost)) {
                        log_ghost_move(ghost->id, ghost->boredom, from_room, target_room->name);
                    }
                }
            } else {
                log_ghost_idle(ghost->id, ghost->boredom, ghost->current_room->name);
            }
            break;
    }
}

void* ghost_thread(void* arg) {
    struct Ghost* ghost = (struct Ghost*)arg;
    if (!ghost) return NULL;
    printf("Ghost %d thread started\n", ghost->id);
    while (ghost->is_running) {
        ghost_update_stats(ghost);
        if (ghost_check_exit_condition(ghost)) {
            break;
        }
        ghost_take_action(ghost);
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 150000000L };
        nanosleep(&ts, NULL);
    }
    printf("Ghost %d thread exiting\n", ghost->id);
    return NULL;
}

EvidenceByte ghost_get_evidence_requirements(struct Ghost* ghost) {
    if (!ghost) return 0;
    return (EvidenceByte)ghost->type;
}
