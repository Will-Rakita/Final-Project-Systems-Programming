#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "defs.h"
#include "helpers.h"

void room_init(struct Room* room, const char* name, bool is_exit) {
    if (!room) return;
    strncpy(room->name, name, MAX_ROOM_NAME - 1);
    room->name[MAX_ROOM_NAME - 1] = '\0';
    room->connection_count = 0;
    room->hunter_count = 0;
    room->ghost = NULL;
    room->evidence = 0;
    room->is_exit = is_exit;
    if (sem_init(&room->sem, 0, 1) != 0) {
        fprintf(stderr, "Failed to initialize room semaphore\n");
    }
}

void rooms_connect(struct Room* a, struct Room* b) {
    if (!a || !b) return;
    if (a->connection_count >= MAX_CONNECTIONS) {
        fprintf(stderr, "WARNING: Room '%s' connections exceed MAX_CONNECTIONS (%d)\n", a->name, MAX_CONNECTIONS);
        return;
    }
    if (b->connection_count >= MAX_CONNECTIONS) {
        fprintf(stderr, "WARNING: Room '%s' connections exceed MAX_CONNECTIONS (%d)\n", b->name, MAX_CONNECTIONS);
        return;
    }
    a->connections[a->connection_count++] = b;
    b->connections[b->connection_count++] = a;
}

bool room_add_hunter(struct Room* room, struct Hunter* hunter) {
    if (!room || !hunter) return false;
    sem_wait(&room->sem);
    if (room->hunter_count >= MAX_ROOM_OCCUPANCY) {
        sem_post(&room->sem);
        return false;
    }
    room->hunters[room->hunter_count++] = hunter;
    hunter->current_room = room;
    sem_post(&room->sem);
    return true;
}

void room_remove_hunter(struct Room* room, struct Hunter* hunter) {
    if (!room || !hunter) return;
    sem_wait(&room->sem);
    for (int i = 0; i < room->hunter_count; i++) {
        if (room->hunters[i] == hunter) {
            for (int j = i; j < room->hunter_count - 1; j++) {
                room->hunters[j] = room->hunters[j + 1];
            }
            room->hunter_count--;
            break;
        }
    }
    hunter->current_room = NULL;
    sem_post(&room->sem);
}

void room_set_ghost(struct Room* room, struct Ghost* ghost) {
    if (!room) return;
    sem_wait(&room->sem);
    room->ghost = ghost;
    if (ghost) {
        ghost->current_room = room;
    }
    sem_post(&room->sem);
}

void room_remove_ghost(struct Room* room) {
    if (!room) return;
    sem_wait(&room->sem);
    if (room->ghost) {
        room->ghost->current_room = NULL;
    }
    room->ghost = NULL;
    sem_post(&room->sem);
}

void room_add_evidence(struct Room* room, enum EvidenceType evidence) {
    if (!room) return;
    sem_wait(&room->sem);
    room->evidence |= evidence;
    sem_post(&room->sem);
}

bool room_remove_evidence(struct Room* room, enum EvidenceType evidence) {
    if (!room) return false;
    sem_wait(&room->sem);
    bool had_evidence = (room->evidence & evidence) != 0;
    if (had_evidence) {
        room->evidence &= ~evidence;
    }
    sem_post(&room->sem);
    return had_evidence;
}

bool room_has_evidence(struct Room* room, enum EvidenceType evidence) {
    if (!room) return false;
    sem_wait(&room->sem);
    bool result = (room->evidence & evidence) != 0;
    sem_post(&room->sem);
    return result;
}

// To avoid re-locking in room_move_entity, we can inline logic instead of calling the above functions.
bool room_move_entity(struct Room* from, struct Room* to, void* entity) {
    if (!from || !to || !entity) return false;
    struct Room* first = (from < to) ? from : to;
    struct Room* second = (from < to) ? to : from;
    sem_wait(&first->sem);
    sem_wait(&second->sem);
    bool can_move = true;
    if (entity != from->ghost) {
        if (to->hunter_count >= MAX_ROOM_OCCUPANCY) {
            can_move = false;
        }
    }
    if (can_move) {
        if (entity == from->ghost) {
            from->ghost = NULL;
            to->ghost = entity;
            ((struct Ghost*)entity)->current_room = to;
        } else {
            // Inline remove/add, not re-locking
            // Remove from 'from'
            for (int i = 0; i < from->hunter_count; i++) {
                if (from->hunters[i] == entity) {
                    for (int j = i; j < from->hunter_count - 1; j++) {
                        from->hunters[j] = from->hunters[j + 1];
                    }
                    from->hunter_count--;
                    break;
                }
            }
            // Add to 'to'
            to->hunters[to->hunter_count++] = (struct Hunter*)entity;
            ((struct Hunter*)entity)->current_room = to;
        }
    }
    sem_post(&second->sem);
    sem_post(&first->sem);
    return can_move;
}

struct Room* room_get_random_connection(struct Room* room) {
    if (!room || room->connection_count == 0) return NULL;
    int index = rand_int_threadsafe(0, room->connection_count);
    return room->connections[index];
}

void room_cleanup(struct Room* room) {
    if (!room) return;
    sem_destroy(&room->sem);
}