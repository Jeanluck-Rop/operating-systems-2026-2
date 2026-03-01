#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 200 // Simulate processes


/* Process descriptor */
typedef struct {
  int id;            // Process ID
  int arrival;       // Arrival time (t0, t1... tn)
  int burst;         // Original CPU burst (cpu time)
  int priority;      // For priority algorithms
  int remaining;     // Remaining Time (para RR, SRTN)
  int start_time;    // First moment when execute
  int finish_time;   // Finish moment 
  int waiting_time;  // Total waiting time
  int turnaround;    // finish_time - arrival
  int response_time; // First moment tha the process receives CPU − arrival
} Process;


/* Enum for handle flag input */
typedef enum {
  GUARANTEED,
  SPN,
  UNKNOWN
} Algorithm;


#endif /* SCHEDULER_H */
