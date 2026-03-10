#ifndef DISPLAY_H
#define DISPLAY_H

/* ─────────────────────────────────────────────────────────────────────────────
 * display.h
 *
 * Pretty-printing utilities for the Banker's Algorithm state.
 * ───────────────────────────────────────────────────────────────────────────── */

#include "banker.h"

/**
 * print_state()
 * Prints the full system state:
 *   Allocation | Max | Need | Available
 */
void print_state(const BankerState *state);

/**
 * print_vector()
 * Prints a single integer vector with a label.
 * Example:  "Request     : [ 0  1  1  0 ]"
 */
void print_vector(const char *label, const int *vec, int len);

/**
 * print_safe_sequence()
 * Prints the safe sequence as  <T0, T2, T1, ...>
 */
void print_safe_sequence(const int *seq, int n);

/**
 * print_request_result()
 * Prints a formatted result banner for a resource request.
 */
void print_request_result(int thread_id, const int *request, int len,
                          BankerStatus status);

#endif /* DISPLAY_H */
