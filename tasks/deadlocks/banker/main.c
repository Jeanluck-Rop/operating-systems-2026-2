#include <stdio.h>
#include <string.h>
#include "banker.h"
#include "display.h"


/* Populates the BankerState with the exercise data and computes Need */
static void
init_state(BankerState *bs)
{
  bs->n_threads   = 5;
  bs->n_resources = 4;

  //Available
  int avail[] = {2, 2, 2, 4};
  memcpy(bs->available, avail, sizeof(avail));
  //Allocation
  int alloc[][4] = {
    {3, 1, 4, 1}, //T0
    {2, 1, 0, 2}, //T1
    {2, 4, 1, 3}, //T2
    {4, 1, 1, 0}, //T3
    {2, 2, 2, 1}  //T4
  };
  
  for (int i = 0; i < bs->n_threads; i++)
    memcpy(bs->allocation[i], alloc[i], sizeof(alloc[i]));

  //Max
  int max[][4] = {
    {6, 4, 7, 3}, // T0
    {4, 2, 3, 2}, // T1
    {2, 5, 3, 3}, // T2
    {6, 3, 3, 2}, // T3
    {5, 6, 7, 5}  // T4
  };
  
  for (int i = 0; i < bs->n_threads; i++)
    memcpy(bs->max[i], max[i], sizeof(max[i]));

  //Need = Max - Allocation 
  compute_need(bs);
}

/*
 * Runs a single request scenario against a FRESH copy of the initial state,
 * so each question (b/c/d) is independent.
 */
static void
check_request(const BankerState *initial,
	      int thread_id,
	      const int *request,
	      const char *label)
{
  //Work on a local copy so the initial state is never modified
  BankerState tmp = *initial;

  BankerStatus status = request_resources(&tmp, thread_id, request);
  print_request_result(thread_id, request, initial->n_resources, status);
  
  //If granted, show the new state and its safe sequence
  if (status == BANKER_OK) {
    int seq[MAX_THREADS];
    printf("  Updated state after granting %s:\n\n", label);
    print_state(&tmp);
    is_safe(&tmp, seq); //recompute to obtain sequence
    print_safe_sequence(seq, tmp.n_threads);
  }
  putchar('\n');
}


/*
 * Banker's Algorithm:
 * 5 threads (T0–T4), 4 resource types (A B C D)
 * Available = { 2, 2, 2, 4 }
 *
 *   Allocation:          Max:
 *   T0  3 1 4 1          6 4 7 3
 *   T1  2 1 0 2          4 2 3 2
 *   T2  2 4 1 3          2 5 3 3
 *   T3  4 1 1 0          6 3 3 2
 *   T4  2 2 2 1          5 6 7 5
 *
 * Questions answered (same as the solved exercise):
 *   a) Show the system is in a safe state
 *   b) T4 requests (2,2,2,4) — can it be granted immediately?
 *   c) T2 requests (0,1,1,0) — can it be granted immediately?
 *   d) T3 requests (2,2,1,2) — can it be granted immediately?
 *
 * Note: each request in (b)–(d) is evaluated against the INITIAL state
 */
int
main(void)
{
  BankerState state;
  init_state(&state);
  
  //Display initial state
  printf("\n+================================================+\n");
  printf("+        Banker's Algorithm — Initial State      +\n");
  printf("+================================================+\n\n");
  print_state(&state);
  
  //a) Verify initial state is safe
  printf("\n - a) Safety check on initial state\n");
  int seq[MAX_THREADS];
  if (is_safe(&state, seq)) {
    printf("  The system is in a " "\033[32m\033[1m" "SAFE STATE" "\033[0m" ".\n");
    print_safe_sequence(seq, state.n_threads);
  } else {
    printf("  The system is in an " "\033[31m\033[1m" "UNSAFE STATE" "\033[0m" ".\n");
  }
  
  //b) T4 requests (2,2,2,4)
  printf("\n - b) T4 requests (2,2,2,4)\n");
  int req_b[] = {2, 2, 2, 4};
  check_request(&state, 4, req_b, "b)");
  
  //c) T2 requests (0,1,1,0)
  printf("\n - c) T2 requests (0,1,1,0)\n");
  int req_c[] = {0, 1, 1, 0};
  check_request(&state, 2, req_c, "c)");

  //d) T3 requests (2,2,1,2)
  printf("\n - d) T3 requests (2,2,1,2)\n");
  int req_d[] = {2, 2, 1, 2};
  check_request(&state, 3, req_d, "d)");
  
  return 0;
}
