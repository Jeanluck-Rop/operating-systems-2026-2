#ifndef BANKER_H
#define BANKER_H

#include <stdbool.h>

#define MAX_THREADS 10
#define MAX_RESOURCES 10

/**
 * Banker's Algorithm - Deadlock Avoidance
 * Based on: Operating System Concepts, Silberschatz et al., Chapter 8
 *
 * Data structures (per book, Section 8.6.3):
 *   Available  [m]    - number of free instances of each resource type
 *   Max        [n][m] - maximum demand declared by each thread
 *   Allocation [n][m] - resources currently allocated to each thread
 *   Need       [n][m] - remaining need: Need[i][j] = Max[i][j] - Allocation[i][j]
 */

/* Error codes */
typedef enum {
    BANKER_OK =  0,
    BANKER_ERR_EXCEEDS_MAX = -1, //Request > Need[i]
    BANKER_ERR_UNAVAILABLE = -2, //Request > Available
    BANKER_ERR_UNSAFE = -3,      //Resulting state unsafe
    BANKER_ERR_INVALID = -4      //Bad argument
} BankerStatus;

/* System state */
typedef struct {
  int n_threads;   //Number of threads
  int n_resources; //Number of resource types
  int available[MAX_RESOURCES];
  int max[MAX_THREADS][MAX_RESOURCES];
  int allocation[MAX_THREADS][MAX_RESOURCES];
  int need[MAX_THREADS][MAX_RESOURCES];
} BankerState;

/**
 * Fills state->need from state->max and state->allocation.
 * Must be called once after initialising max and allocation.
 */
void compute_need(BankerState *state);

/**
 * Safety Algorithm.
 * Returns true if the system is in a safe state.
 * If safe_seq != NULL, writes the safe sequence of thread indices into it
 * (caller must provide an array of length state->n_threads).
 */
bool is_safe(const BankerState *state, int safe_seq[]);

/**
 * Resource-Request Algorithm.
 * Attempts to grant request[0..n_resources-1] for thread_id.
 * Returns:
 *   BANKER_OK              - request granted, state updated
 *   BANKER_ERR_EXCEEDS_MAX - request exceeds thread's declared maximum
 *   BANKER_ERR_UNAVAILABLE - not enough resources available right now
 *   BANKER_ERR_UNSAFE      - granting would lead to unsafe state
 *   BANKER_ERR_INVALID     - invalid thread_id or NULL pointer
 */
BankerStatus request_resources(BankerState *state, int thread_id, const int request[]);

/**
 * Releases request[0..n_resources-1] for thread_id back to the system.
 * Returns BANKER_OK on success, BANKER_ERR_INVALID on bad arguments.
 */
BankerStatus release_resources(BankerState *state, int thread_id, const int release[]);

#endif //BANKER_H
