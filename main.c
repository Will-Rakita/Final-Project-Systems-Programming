#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "defs.h"
#include "helpers.h"

int main() {
    struct House* house = NULL;
    pthread_t ghost_thread;
    pthread_t* hunter_threads = NULL;
    
    printf("=== Ghost Hunter Simulation Starting ===\n");
    
    // 1. Initialize a House structure
    house = house_init();
    if (!house) {
        fprintf(stderr, "Failed to initialize house\n");
        return EXIT_FAILURE;
    }
    
    // 2. Populate the House with rooms using the provided helper function
    house_populate_rooms(house);
    printf("House populated with %d rooms\n", house->room_count);
    
    // 3. Initialize the ghost data
    if (ghost_init(house) == NULL) {
        fprintf(stderr, "Failed to initialize ghost\n");
        house_cleanup(house);
        return EXIT_FAILURE;
    }
    
    // 4. Initialize hunters based on user input
    printf("\n--- Hunter Registration ---\n");
    printf("Enter hunter details (type 'done' for name to finish):\n");
    
    char name_buffer[MAX_HUNTER_NAME];
    int hunter_id;
    
    while (house->hunter_count < MAX_ROOM_OCCUPANCY) { // Practical limit
        printf("\nHunter name: ");
        if (scanf("%63s", name_buffer) != 1) break;
        
        if (strcmp(name_buffer, "done") == 0) {
            break;
        }
        
        printf("Hunter ID: ");
        if (scanf("%d", &hunter_id) != 1) {
            fprintf(stderr, "Invalid ID input\n");
            continue;
        }
        
        // Create and initialize the hunter
        struct Hunter* new_hunter = hunter_init(name_buffer, hunter_id, house);
        if (!new_hunter) {
            fprintf(stderr, "Failed to create hunter %s\n", name_buffer);
            continue;
        }
        
        // Add to house collection
        hunter_collection_append(house, new_hunter);
        
        // Log hunter initialization
        log_hunter_init(hunter_id, house->starting_room->name, name_buffer, new_hunter->device);
    }
    
    if (house->hunter_count == 0) {
        printf("No hunters created. Simulation ending.\n");
        house_cleanup(house);
        return EXIT_SUCCESS;
    }
    
    printf("\n--- Starting Simulation with %d hunters ---\n", house->hunter_count);
    
    // 5. Create threads for the ghost and each hunter
    hunter_threads = malloc(sizeof(pthread_t) * house->hunter_count);
    if (!hunter_threads) {
        fprintf(stderr, "Failed to allocate thread array\n");
        house_cleanup(house);
        return EXIT_FAILURE;
    }
    
    // Create ghost thread
    if (pthread_create(&ghost_thread, NULL, ghost_thread, house->ghost) != 0) {
        fprintf(stderr, "Failed to create ghost thread\n");
        free(hunter_threads);
        house_cleanup(house);
        return EXIT_FAILURE;
    }
    
    // Create hunter threads
    for (int i = 0; i < house->hunter_count; i++) {
        if (pthread_create(&hunter_threads[i], NULL, hunter_thread, house->hunters[i]) != 0) {
            fprintf(stderr, "Failed to create hunter thread %d\n", i);
            // Mark all previously created threads for cleanup
            for (int j = 0; j < i; j++) {
                house->hunters[j]->is_running = false;
            }
            house->ghost->is_running = false;
            break;
        }
    }
    
    // 6. Wait for all threads to complete
    printf("\n--- Simulation Running ---\n");
    
    // Wait for ghost thread
    pthread_join(ghost_thread, NULL);
    
    // Wait for all hunter threads
    for (int i = 0; i < house->hunter_count; i++) {
        if (hunter_threads[i]) {
            pthread_join(hunter_threads[i], NULL);
        }
    }
    
    free(hunter_threads);
    
    printf("\n--- Simulation Complete ---\n");
    
    // 7. Print final results to the console
    printf("\n=== FINAL RESULTS ===\n");
    
    // Ghost type encountered
    printf("Ghost Type: %s\n", ghost_to_string(house->ghost->type));
    printf("Ghost ID: %d\n", house->ghost->id);
    printf("Ghost exited due to: %s\n", 
           house->ghost->boredom > ENTITY_BOREDOM_MAX ? "boredom" : "hunters won");
    
    // Hunter results
    printf("\n--- Hunter Results ---\n");
    for (int i = 0; i < house->hunter_count; i++) {
        struct Hunter* h = house->hunters[i];
        printf("Hunter %d (%s):\n", h->id, h->name);
        printf("  Exit reason: %s\n", exit_reason_to_string(h->exit_reason));
        printf("  Final device: %s\n", evidence_to_string(h->device));
        printf("  Final stats: boredom=%d, fear=%d\n", h->boredom, h->fear);
    }
    
    // Evidence analysis
    printf("\n--- Evidence Analysis ---\n");
    printf("Collected evidence bits: 0x%02X\n", house->case_file.collected);
    printf("Evidence matches known ghost: %s\n", 
           evidence_is_valid_ghost(house->case_file.collected) ? "YES" : "NO");
    
    // Check what ghost the evidence suggests
    const enum GhostType* ghost_types = NULL;
    int ghost_count = get_all_ghost_types(&ghost_types);
    enum GhostType matched_ghost = 0;
    
    for (int i = 0; i < ghost_count; i++) {
        if (house->case_file.collected == (EvidenceByte)ghost_types[i]) {
            matched_ghost = ghost_types[i];
            break;
        }
    }
    
    if (matched_ghost != 0) {
        printf("Evidence identifies ghost as: %s\n", ghost_to_string(matched_ghost));
        printf("Correct identification: %s\n", 
               matched_ghost == house->ghost->type ? "YES" : "NO");
    } else {
        printf("Evidence is insufficient or inconsistent to identify a specific ghost.\n");
    }
    
    // 8. Clean up all dynamically allocated resources
    house_cleanup(house);
    
    printf("\n=== Simulation Cleanup Complete ===\n");
    return EXIT_SUCCESS;
}