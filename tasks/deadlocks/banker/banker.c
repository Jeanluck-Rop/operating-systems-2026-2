#include "banker.h"
#include <string.h>

/**
 * Implementation of the Banker's Algorithm.
 * Based on: Operating System Concepts, Silberschatz et al., Chapter 8
 */

/*
 * Returns true if a[j] <= b[j] for all j in [0, len).
 * Corresponds to the book's "X <= Y" notation (§8.6.3).
 */
static bool
vec_leq(const int *a,
	const int *b,
	int len)
{
  for (int j = 0; j < len; j++)
    if (a[j] > b[j])
      return false;
  return true;
}

/*
 * Need[i][j] = Max[i][j] - Allocation[i][j]  (book §8.6.3)
 */
void
compute_need(BankerState *state)
{
  for (int i = 0; i < state->n_threads; i++)
    for (int j = 0; j < state->n_resources; j++)
      state->need[i][j] = state->max[i][j] - state->allocation[i][j];
}

/*
 * Safety Algorithm
 * Step 1: Work = Available; Finish[i] = false for all i
 * Step 2: Find i such that Finish[i]==false AND Need[i] <= Work
 * Step 3: Work += Allocation[i]; Finish[i] = true  → go to step 2
 * Step 4: If all Finish[i]==true → SAFE
 */
bool
is_safe(const BankerState *state,
	int safe_seq[])
{
    const int n = state->n_threads;
    const int m = state->n_resources;
    
    int  work  [MAX_RESOURCES];
    bool finish[MAX_THREADS];

    //Step 1
    memcpy(work, state->available, sizeof(int) * m);
    for (int i = 0; i < n; i++)
      finish[i] = false;
    
    int seq_idx = 0; //position in safe sequence
    int found_count = 0;
    
    //Steps 2–3: repeat until no thread can be scheduled
    while (found_count < n) {
      bool found = false;
      
      for (int i = 0; i < n; i++) {
	//Step 2: candidate must be unfinished AND Need[i] <= Work
	if (!finish[i] && vec_leq(state->need[i], work, m)) {
	  //Step 3: simulate thread i completing
	  for (int j = 0; j < m; j++)
	    work[j] += state->allocation[i][j];
	  
	  finish[i] = true;
	  found = true;
	  found_count++;
	  
	  if (safe_seq)
	    safe_seq[seq_idx++] = i;
	}
      }
      
      //No thread could be scheduled in this pass -> unsafe
      if (!found)
	return false;
    }
    
    //Step 4: all threads finished -> safe
    return true;
}

/*
 * Resource-Request Algorithm — book §8.6.3.2
 */
BankerStatus
request_resources(BankerState *state,
		  int thread_id,
		  const int request[])
{
  if (!state || !request || thread_id < 0 || thread_id >= state->n_threads)
    return BANKER_ERR_INVALID;
  
  const int m = state->n_resources;
  
  //Step 1: Request[i] <= Need[i]
  if (!vec_leq(request, state->need[thread_id], m))
    return BANKER_ERR_EXCEEDS_MAX;
  
  //Step 2: Request[i] <= Available
  if (!vec_leq(request, state->available, m))
    return BANKER_ERR_UNAVAILABLE;
  
  //Step 3: Pretend to allocate - modify state temporarily
  for (int j = 0; j < m; j++) {
    state->available        [j]          -= request[j];
    state->allocation[thread_id][j]      += request[j];
    state->need      [thread_id][j]      -= request[j];
  }
  
  //Step 4: Run safety algorithm on the simulated state
  if (is_safe(state, NULL))
    return BANKER_OK; //Safe -> keep the allocation
  
  //Unsafe -> roll back to original state
  for (int j = 0; j < m; j++) {
    state->available        [j]          += request[j];
    state->allocation[thread_id][j]      -= request[j];
    state->need      [thread_id][j]      += request[j];
  }
  
  return BANKER_ERR_UNSAFE;
}

/*
 * Thread thread_id voluntarily releases resources back to the system.
 */
BankerStatus
release_resources(BankerState *state,
		  int thread_id,
		  const int release[])
{
  if (!state || !release || thread_id < 0 || thread_id >= state->n_threads)
    return BANKER_ERR_INVALID;
  
  const int m = state->n_resources;
  
  for (int j = 0; j < m; j++) {
    //Guard: cannot release more than currently allocated
    if (release[j] > state->allocation[thread_id][j])
      return BANKER_ERR_INVALID;
    
    state->allocation[thread_id][j] -= release[j];
    state->available[j] += release[j];
    state->need[thread_id][j] += release[j];
  }
  
  return BANKER_OK;
}
