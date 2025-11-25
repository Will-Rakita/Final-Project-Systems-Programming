#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "helpers.h"

EvidenceByte evidence_add(EvidenceByte mask, enum EvidenceType evidence) {
    return mask | evidence;
}

EvidenceByte evidence_remove(EvidenceByte mask, enum EvidenceType evidence) {
    return mask & ~evidence;
}

bool evidence_contains(EvidenceByte mask, enum EvidenceType evidence) {
    return (mask & evidence) != 0;
}

int evidence_count_unique(EvidenceByte mask) {
    int count = 0;
    const enum EvidenceType* evidence_types = NULL;
    int type_count = get_all_evidence_types(&evidence_types);
    
    for (int i = 0; i < type_count; i++) {
        if (evidence_contains(mask, evidence_types[i])) {
            count++;
        }
    }
    return count;
}

bool evidence_has_three_unique(EvidenceByte mask) {
    return evidence_count_unique(mask) >= 3;
}

enum EvidenceType evidence_get_random_type() {
    const enum EvidenceType* evidence_types = NULL;
    int count = get_all_evidence_types(&evidence_types);
    if (count == 0) return 0;
    int index = rand_int_threadsafe(0, count);
    return evidence_types[index];
}

void casefile_init(struct CaseFile* case_file) {
    if (!case_file) return;
    
    case_file->collected = 0;
    case_file->solved = false;
    
    if (sem_init(&case_file->mutex, 0, 1) != 0) {
        fprintf(stderr, "Failed to initialize case file semaphore\n");
    }
}
void casefile_add_evidence(struct CaseFile* case_file, enum EvidenceType evidence) {
    if (!case_file) return;
    
    sem_wait(&case_file->mutex);
    case_file->collected |= evidence;
    // Change to: solved when we have 3 unique evidence types (spec R-3)
    if (evidence_count_unique(case_file->collected) >= 3) {
        case_file->solved = true;
    }
    sem_post(&case_file->mutex);
}

bool casefile_is_solved(struct CaseFile* case_file) {
    if (!case_file) return false;
    
    sem_wait(&case_file->mutex);
    bool solved = case_file->solved;
    sem_post(&case_file->mutex);
    return solved;
}

EvidenceByte casefile_get_evidence(struct CaseFile* case_file) {
    if (!case_file) return 0;
    
    sem_wait(&case_file->mutex);
    EvidenceByte collected = case_file->collected;
    sem_post(&case_file->mutex);
    return collected;
}

void casefile_cleanup(struct CaseFile* case_file) {
    if (!case_file) return;
    sem_destroy(&case_file->mutex);
}