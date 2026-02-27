#include <stdio.h>
#include <stdlib.h>

#define MAX_PROCESSES 200 // Simulate processes

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


/* Prints per-process metrics and averages for Wait, Turnaround and Response time */
void
print_metrics(Process *p,
	      int n)
{
  double total_waiting = 0;
  double total_turnaround = 0;
  double total_response = 0;
    
  printf("\nID | Arrival | Burst | Start | Finish | Wait | Turnaround | Response\n");
  printf("----------------------------------------------------------------------\n");
  
  for(int i = 0; i < n; i++) {
    printf("%2d | %7d | %5d | %5d | %6d | %4d | %10d | %8d\n",
	   p[i].id, p[i].arrival, p[i].burst, p[i].start_time, 
	   p[i].finish_time, p[i].waiting_time, p[i].turnaround, p[i].response_time);

    //Accumulate totals so we can compute averages after the loop
    total_waiting += p[i].waiting_time;
    total_turnaround += p[i].turnaround;
    total_response += p[i].response_time;
  }

  //Divide by n to get the mean value for each metric across all processes
  printf("\nAverages -> Wait: %.2f | Turnaround: %.2f | Response: %.2f\n",
	 total_waiting / n, total_turnaround / n, total_response / n);
}


/**
 * Guaranteed Scheduling
 * Gives ~1/n of CPU to each process measuring
 * (CPU actually consumed) / (CPU it was entitled to)
 **/
void
simulate_guaranteed_scheduling(Process *p,
			       int n)
{
  int current_time = 0; //global clock, advances one tick at a time
  int end_scheduling = 0; //counter of finished processes
  double cpu_consumed[MAX_PROCESSES]; //for track how many CPU each process has received
  for (int i = 0; i < n; i++)
    cpu_consumed[i] = 0.0;

  printf("\n--- Initiating Guaranteed Scheduling ---\n");

  //Main loop, keeps running until every process has finished
  while (end_scheduling < n) {
    
    //Count active processes (arrived and not finished)
    int active_processes = 0;
    //A process is active if it has already arrived (arrival <= current_time)
    // and still has CPU work left (remaining > 0)
    for (int i = 0; i < n; i++)
      if (p[i].arrival <= current_time && p[i].remaining > 0)
        active_processes++;
    //No proccesses active, we try again on the next tick (busy-wait / idle advance)
    if (active_processes == 0) {
      current_time++;
      continue;
    }

    // Now we look for the process with the lowest consumed/entitled ratio
    int unfavored_process = -1;
    double min_ratio = 9999999.0; //will be replaced by the first real ratio
    for (int i = 0; i < n; i++) {
      //Skip processes that have not arrived yet or have already finished
      if (p[i].arrival > current_time || p[i].remaining <= 0)
        continue;

      //How long this process has been in the system
      double time_since_arrival = (double)(current_time - p[i].arrival);
      if (time_since_arrival == 0)
	time_since_arrival = 1.0; //to avoid a 0/n ratio on the very first tick

      double time_entitled = time_since_arrival / (double)active_processes;

      //How much CPU it got vs how much it deserved.
      //ratio < 1: process is deprived, higher priority
      //ratio > 1: process privileged, lower priority
      double ratio = (time_entitled <= 0) ? 0.0 : cpu_consumed[i] / time_entitled;

      //Keep track of the process with the smallest ratio
      if (ratio < min_ratio) {
	min_ratio = ratio;
	unfavored_process = i;
      }
    }
    
    //Record first-time scheduling info
    if (p[unfavored_process].start_time == -1) {
      p[unfavored_process].start_time = current_time;
      //Delay between arrival and first CPU access
      p[unfavored_process].response_time = current_time - p[unfavored_process].arrival;
    }

    cpu_consumed[unfavored_process]++; //give one CPU tick to the unfavored process
    p[unfavored_process].remaining--; //one tick of work done, one less to go
    current_time++;
    
    //Check if the process finished all of its work and mark it done
    if (p[unfavored_process].remaining == 0) {
      p[unfavored_process].finish_time = current_time;
      p[unfavored_process].turnaround = p[unfavored_process].finish_time - p[unfavored_process].arrival; //total time from arrival to completion
      p[unfavored_process].waiting_time = p[unfavored_process].turnaround - p[unfavored_process].burst; //time spent NOT running: turnaround - actual work
      end_scheduling++; 
    }
  }
}


/* */
void
shortest_process_next(Process *p,
		      int n)
{
  //
}


/* */
int
main(char* flags)
{
  Process processes[MAX_PROCESSES];
  int n = 0;

  // Read since stdin to EOF, each line must have: id arrival burst priority
  while (scanf("%d %d %d %d",
	       &processes[n].id,
	       &processes[n].arrival,
	       &processes[n].burst,
	       &processes[n].priority) == 4) {

    processes[n].remaining     = processes[n].burst;
    processes[n].start_time    = -1;
    processes[n].finish_time   = 0;
    processes[n].waiting_time  = 0;
    processes[n].turnaround    = 0;
    processes[n].response_time = -1;
    n++;

    if (n >= MAX_PROCESSES) {
      fprintf(stderr, "MAX_PROCESSES reached=%d\n", MAX_PROCESSES);
      break;
    }
  }

  printf("%d processes readed:\n", n);
  for (int i = 0; i < n; i++) {
    printf("Process: %3d:   arrival: %3d   burst: %2d   priority: %d\n",
	   processes[i].id,
	   processes[i].arrival,
	   processes[i].burst,
	   processes[i].priority);
  }

  if (flags == "-g") {
    simulate_guaranteed_scheduling(processes, n);
    print_metrics(processes, n);
  }
  else if (flags == "-s") {
    simulate_guaranteed_scheduling(processes, n);
    print_metrics(processes, n);
  }
  else {
    undefined;
  }
  
  
  return 0;
}
