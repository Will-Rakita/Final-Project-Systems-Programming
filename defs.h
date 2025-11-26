#ifndef DEFS_H
#define DEFS_H

#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>

#define MAX_ROOM_NAME 64
#define MAX_HUNTER_NAME 64
#define MAX_ROOMS 24
#define MAX_ROOM_OCCUPANCY 8
#define MAX_CONNECTIONS 8
#define ENTITY_BOREDOM_MAX 15
#define HUNTER_FEAR_MAX 15
#define DEFAULT_GHOST_ID 68057

typedef unsigned char EvidenceByte;

enum LogReason {
    LR_EVIDENCE = 0,
    LR_BORED = 1,
    LR_AFRAID = 2
};

enum EvidenceType {
    EV_EMF          = 1 << 0,
    EV_ORBS         = 1 << 1,
    EV_RADIO        = 1 << 2,
    EV_TEMPERATURE  = 1 << 3,
    EV_FINGERPRINTS = 1 << 4,
    EV_WRITING      = 1 << 5,
    EV_INFRARED     = 1 << 6,
};

enum GhostType {
    GH_POLTERGEIST  = EV_FINGERPRINTS | EV_TEMPERATURE | EV_WRITING,
    GH_THE_MIMIC    = EV_FINGERPRINTS | EV_TEMPERATURE | EV_RADIO,
    GH_HANTU        = EV_FINGERPRINTS | EV_TEMPERATURE | EV_ORBS,
    GH_JINN         = EV_FINGERPRINTS | EV_TEMPERATURE | EV_EMF,
    GH_PHANTOM      = EV_FINGERPRINTS | EV_INFRARED    | EV_RADIO,
    GH_BANSHEE      = EV_FINGERPRINTS | EV_INFRARED    | EV_ORBS,
    GH_GORYO        = EV_FINGERPRINTS | EV_INFRARED    | EV_EMF,
    GH_BULLIES      = EV_FINGERPRINTS | EV_WRITING     | EV_RADIO,
    GH_MYLING       = EV_FINGERPRINTS | EV_WRITING     | EV_EMF,
    GH_OBAKE        = EV_FINGERPRINTS | EV_ORBS        | EV_EMF,
    GH_YUREI        = EV_TEMPERATURE  | EV_INFRARED    | EV_ORBS,
    GH_ONI          = EV_TEMPERATURE  | EV_INFRARED    | EV_EMF,
    GH_MOROI        = EV_TEMPERATURE  | EV_WRITING     | EV_RADIO,
    GH_REVENANT     = EV_TEMPERATURE  | EV_WRITING     | EV_ORBS,
    GH_SHADE        = EV_TEMPERATURE  | EV_WRITING     | EV_EMF,
    GH_ONRYO        = EV_TEMPERATURE  | EV_RADIO       | EV_ORBS,
    GH_THE_TWINS    = EV_TEMPERATURE  | EV_RADIO       | EV_EMF,
    GH_DEOGEN       = EV_INFRARED     | EV_WRITING     | EV_RADIO,
    GH_THAYE        = EV_INFRARED     | EV_WRITING     | EV_ORBS,
    GH_YOKAI        = EV_INFRARED     | EV_RADIO       | EV_ORBS,
    GH_WRAITH       = EV_INFRARED     | EV_RADIO       | EV_EMF,
    GH_RAIJU        = EV_INFRARED     | EV_ORBS        | EV_EMF,
    GH_MARE         = EV_WRITING      | EV_RADIO       | EV_ORBS,
    GH_SPIRIT       = EV_WRITING      | EV_RADIO       | EV_EMF,
};

struct CaseFile {
    EvidenceByte collected;
    bool solved;
    sem_t mutex;
};

struct RoomNode {
    struct Room* room;
    struct RoomNode* next;
};

struct RoomStack {
    struct RoomNode* head;
};

struct Room {
    char name[MAX_ROOM_NAME];
    struct Room* connections[MAX_CONNECTIONS];
    int connection_count;
    struct Hunter* hunters[MAX_ROOM_OCCUPANCY];
    int hunter_count;
    struct Ghost* ghost;
    EvidenceByte evidence;
    bool is_exit;
    sem_t sem;
};

struct Hunter {
    char name[MAX_HUNTER_NAME];
    int id;
    struct Room* current_room;
    struct House* house;
    struct CaseFile* case_file;
    enum EvidenceType device;
    struct RoomStack path;
    int boredom;
    int fear;
    bool return_to_van;
    bool is_running;
    enum LogReason exit_reason;
};

struct Ghost {
    int id;
    enum GhostType type;
    struct Room* current_room;
    int boredom;
    bool is_running;
};

struct House {
    struct Room rooms[MAX_ROOMS];
    int room_count;
    struct Hunter** hunters;
    int hunter_count;
    int hunter_capacity;
    struct CaseFile case_file;
    struct Ghost* ghost;
    struct Room* starting_room;
};

// House functions
struct House* house_init();
void house_cleanup(struct House* house);
void hunter_collection_append(struct House* house, struct Hunter* hunter);

// Room functions
void room_init(struct Room* room, const char* name, bool is_exit);
void rooms_connect(struct Room* a, struct Room* b);
bool room_add_hunter(struct Room* room, struct Hunter* hunter);
void room_remove_hunter(struct Room* room, struct Hunter* hunter);
void room_set_ghost(struct Room* room, struct Ghost* ghost);
void room_remove_ghost(struct Room* room);
void room_add_evidence(struct Room* room, enum EvidenceType evidence);
bool room_remove_evidence(struct Room* room, enum EvidenceType evidence);
bool room_has_evidence(struct Room* room, enum EvidenceType evidence);
bool room_move_entity(struct Room* from, struct Room* to, void* entity);
struct Room* room_get_random_connection(struct Room* room);
void room_cleanup(struct Room* room);

// Evidence functions
EvidenceByte evidence_add(EvidenceByte mask, enum EvidenceType evidence);
EvidenceByte evidence_remove(EvidenceByte mask, enum EvidenceType evidence);
bool evidence_contains(EvidenceByte mask, enum EvidenceType evidence);
int evidence_count_unique(EvidenceByte mask);
bool evidence_has_three_unique(EvidenceByte mask);
enum EvidenceType evidence_get_random_type();
void casefile_init(struct CaseFile* case_file);
void casefile_add_evidence(struct CaseFile* case_file, enum EvidenceType evidence);
bool casefile_is_solved(struct CaseFile* case_file);
EvidenceByte casefile_get_evidence(struct CaseFile* case_file);
void casefile_cleanup(struct CaseFile* case_file);

// RoomStack functions
void roomstack_init(struct RoomStack* stack);
void roomstack_push(struct RoomStack* stack, struct Room* room);
struct Room* roomstack_pop(struct RoomStack* stack);
void roomstack_clear(struct RoomStack* stack);
bool roomstack_is_empty(struct RoomStack* stack);

// Hunter functions
struct Hunter* hunter_init(const char* name, int id, struct House* house);
void hunter_cleanup(struct Hunter* hunter);
void hunter_update_stats(struct Hunter* hunter);
bool hunter_check_exit_conditions(struct Hunter* hunter);
void hunter_van_check(struct Hunter* hunter);
void hunter_gather_evidence(struct Hunter* hunter);
void hunter_move(struct Hunter* hunter);
void hunter_take_turn(struct Hunter* hunter);
void* hunter_thread(void* arg);

// Ghost functions
struct Ghost* ghost_init(struct House* house);
void ghost_cleanup(struct Ghost* ghost);
void ghost_update_stats(struct Ghost* ghost);
bool ghost_check_exit_condition(struct Ghost* ghost);
void ghost_take_action(struct Ghost* ghost);
void ghost_take_turn(struct Ghost* ghost);
void* ghost_thread(void* arg);
EvidenceByte ghost_get_evidence_requirements(struct Ghost* ghost);

#endif
