#include "scheduler.h"
#include <stdio.h>
#include "pico/stdlib.h"

tcb_t tasks[MAX_TASKS];
int current_task = -1;

/**
 * @brief Initialize the task stack so it looks like an interrupted context.
 * @param id Task index in the task table.
 */
void init_task_stack(int id) {
  uint32_t *stack_top = tasks[id].stack + STACK_SIZE;
    
  // Hardware stack frame (simulated)
  *(--stack_top) = 0x01000000; // xPSR (Thumb bit enabled)
  *(--stack_top) = (uint32_t)tasks[id].entry_point; // PC (Program Counter)
  *(--stack_top) = 0xFFFFFFFD; // LR (Return to Thread mode using PSP)
  *(--stack_top) = 0; // R12
  *(--stack_top) = 0; // R3
  *(--stack_top) = 0; // R2
  *(--stack_top) = 0; // R1
  *(--stack_top) = 0; // R0

  // Software stack frame (simulated, R4-R11)
  for (int i = 0; i < 8; i++) {
    *(--stack_top) = 0;
  }
    
  tasks[id].sp = stack_top;
  tasks[id].state = READY;
  //Load initial quantum
  tasks[id].remaining = tasks[id].quantum;
}

/**
 * @brief Register a task in the system without enqueuing it yet.
 * @param id Task index in the task table.
 * @param entry_point Task entry function.
 */
void task_create(int id, void (*entry_point)(void)) {
  if (id >= 0 && id < MAX_TASKS) {
    tasks[id].entry_point = entry_point;
    tasks[id].state = DORMANT;
    //Default quantum
    tasks[id].quantum = 1;
    tasks[id].remaining = 1;
  }
}


//USB comms parser state
//0 = idle
//1 = 'q' received, wait id
//2 = id received, wait quantum (weight)
static int cmd_state = 0;
static int cmd_task_id = -1;

/**
 * @brief SysTick handler called automatically by the hardware.
 */
void isr_systick() {
  // 1. Non-blocking USB UART polling
  int c = getchar_timeout_us(0);

  if (c != PICO_ERROR_TIMEOUT) {
    if (cmd_state == 0) {
      if (c >= '1' && c <= '4') {
	int idx = c - '1';
	if (tasks[idx].state == DORMANT) {
	  init_task_stack(idx); // Prepare its stack
	  printf("[Kernel] Tarea %d agregada a la cola  (peso=%d).\n",
		 idx + 1, tasks[idx].quantum);
	} else {
	  printf("[Kernel] Tarea %d ya esta en ejecucion (peso=%d).\n",
		 idx + 1, tasks[idx].quantum);
	}
      }
      //'q' command init
      else if (c == 'q') {
	printf("[Kernel] Peso > selecciona tarea (1-4): ");
	cmd_state = 1;
      }
    }

    else if (cmd_state == 1) {
      //Waiting task id
      if (c >= '1' && c <= '4') {
	cmd_task_id = c - '1';
	printf("%c\n[Kernel] Peso > nuevo peso para tarea %d (1-9): ",
	       c, cmd_task_id + 1);
	cmd_state = 2;
      } else {
	printf("Cancelado.\n");
	cmd_state = 0;
      }
    }

    else if (cmd_state == 2) {
      //Waiting quantum id
      if (c >= '1' && c <= '9') {
	uint8_t new_quantum = c - '0';
	tasks[cmd_task_id].quantum = new_quantum;
	//If task's running, update the quantum
	if (tasks[cmd_task_id].state == RUNNING)
	  tasks[cmd_task_id].remaining = new_quantum;
	printf("%c\n[Kernel] Tarea %d -> peso=%d (%d ms/quantum).\n",
	       c, cmd_task_id + 1, new_quantum, new_quantum * 10);
      } else {
	printf("Peso invalido, cancelado.\n");
      }
      cmd_state   = 0;
      cmd_task_id = -1;
    }
  }

  //Decrement quantum
  if (current_task != -1 && tasks[current_task].state == RUNNING) {
    if (tasks[current_task].remaining > 1) {
      tasks[current_task].remaining--;
      return;   //task keep CPU, NOT launch pendsv
    }
    //Quantum spent: reload next time
    tasks[current_task].remaining = tasks[current_task].quantum;
  }
    
  // 2. Trigger PendSV to perform the context switch
  // Write to the Interrupt Control and State Register (ICSR)
  *(volatile uint32_t *)0xE000ED04 = (1 << 28);
}

/**
 * @brief Handle the EXIT syscall by terminating the current task.
 */
void k_task_exit(void) {
  if (current_task != -1) {
    tasks[current_task].state = DORMANT;
    printf("Tarea %d terminada.\n", current_task + 1);
    // Force an immediate context switch
    *(volatile uint32_t *)0xE000ED04 = (1 << 28);
  }
}

/**
 * @brief Pick the next runnable task using Round Robin.
 * @param current_sp Stack pointer of the task being switched out.
 * @return Stack pointer of the selected task, or current_sp if none is ready.
 */
uint32_t schedule(uint32_t current_sp) {
  // Save the SP of the task that was just paused
  if (current_task != -1 && tasks[current_task].state == RUNNING) {
    tasks[current_task].sp = (uint32_t *)current_sp;
    tasks[current_task].state = READY; // Put it back in the ready queue
  }

  // Round Robin algorithm
  int next_task = current_task;
  int tries = 0;

  do {
    next_task = (next_task + 1) % MAX_TASKS;
    tries++;
    // If we find a ready task, stop searching
    if (tasks[next_task].state == READY) {
      break;
    }
  } while (tries < MAX_TASKS);

  // If a runnable task was found
  if (tasks[next_task].state == READY) {
    current_task = next_task;
    tasks[current_task].state = RUNNING;
    //Realoaf
    tasks[current_task].remaining = tasks[current_task].quantum;
    return (uint32_t)tasks[current_task].sp;
  }

  // If no task is ready, keep the current context
  return current_sp; 
}
