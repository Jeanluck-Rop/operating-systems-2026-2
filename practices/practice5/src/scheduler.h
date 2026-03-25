#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define MAX_TASKS 5
#define STACK_SIZE 256 // 256 palabras de 32 bits = 1KB por tarea

// Estados posibles de una tarea
typedef enum {
  DORMANT, // Inactiva, no está en la cola
  READY,   // Lista para ejecutarse
  RUNNING, // Ejecutándose actualmente
  BLOCKED  // Esperando mutex
} task_state_t;

// Task Control Block
typedef struct {
  uint32_t *sp;                  // Puntero de Pila actual
  task_state_t state;            // Estado de la tarea
  uint32_t stack[STACK_SIZE];    // Espacio de memoria de la pila
  void (*entry_point)(void);     // Función de la tarea
  uint8_t quantum;   //priority quantum
  uint8_t remaining; //quantum countdown
} tcb_t;

// Semaphore (mutex and counter)
typedef struct {
  int count;            // Available resources (count >= 0)
  int queue[MAX_TASKS]; // FIFO queue of blocked task IDs
  int head;             // Read index
  int tail;             // Write index
  int size;             // Number of currently blocked tasks
} semaphore_t;
 

// Prototipos del planificador
void scheduler_init(void);
void scheduler_init(void);
void task_create(int id, void (*entry_point)(void));
void k_task_exit(void);
uint32_t schedule(uint32_t current_sp);

//Semaphore prototypes
void semaphore_init (semaphore_t *sem, int initial_count);
void sys_semaphore_wait(semaphore_t *sem);
void sys_semaphore_post(semaphore_t *sem);

#endif
