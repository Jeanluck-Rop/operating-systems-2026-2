#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/exception.h"
#include "scheduler.h"

// Registros de control del SysTick (Timer de hardware integrado en ARM Cortex-M)
#define SYSTICK_CTRL (*(volatile uint32_t *)0xE000E010)
#define SYSTICK_LOAD (*(volatile uint32_t *)0xE000E014)
#define SYSTICK_VAL  (*(volatile uint32_t *)0xE000E018)

// Wrappers de Ensamblador (Manejadores de excepciones)
extern void wrapper_svc(void);
extern void isr_pendsv(void);

//extern void init_task_stack(int id);

// Prototipos de las tareas de usuario (definidas en user_app.c)
extern void task_subsystem1(void);
extern void task_subsystem2(void);
extern void task_subsystem3(void);
extern void task_subsystem4(void);
extern void task_subsystem5(void);

/* ------------------------------------------------------------------
 * Semáforos globales — accedidos también desde user_app.c (extern)
 * ------------------------------------------------------------------ */
semaphore_t sem_radio;    // Mutex:   init = 1  → solo 1 tarea en UART
semaphore_t sem_energia;  // Contador: init = 3 → máx. 3 LEDs encendidos

int main() {
  // 1. Inicializar I/O Estándar (Habilita printf y lectura por cable USB)
  stdio_init_all();

  // 2. Registrar los manejadores de Excepciones del Sistema Operativo
  // SVC (ID 11) maneja las Syscalls solicitadas por las tareas
  exception_set_exclusive_handler((enum exception_number)11, wrapper_svc);
  // PendSV (ID 14) maneja el Cambio de Contexto asíncrono
  exception_set_exclusive_handler((enum exception_number)14, isr_pendsv);

  // 3. Inicializar el Process Stack Pointer (PSP) a 0.
  // Esto le avisa al ensamblador que iniciamos en modo Idle (sin tareas de usuario previas)
  __asm volatile ("msr psp, %0" : : "r" (0));

  /* Inicializar semáforos ANTES de encolar las tareas */
  semaphore_init(&sem_radio,   1);  // Mutex
  semaphore_init(&sem_energia, 3);  // Contador
    
  // 4. Registrar tareas en el Scheduler (inician en estado DORMANT, esperando al usuario)
  task_create(0, task_subsystem1);
  task_create(1, task_subsystem2);
  task_create(2, task_subsystem3);
  task_create(3, task_subsystem4);
  task_create(4, task_subsystem5);

  /* --- FASE A: encolar todas al mismo tiempo desde el arranque --- */
  
  // 5. Configurar e iniciar SysTick
  // Para un reloj de 125MHz, contar 1,250,000 ciclos equivale a 10 milisegundos
  SYSTICK_LOAD = 1250000 - 1; 
  SYSTICK_VAL = 0;
  // 0x07 = Habilita el Timer, habilita la Interrupción, y usa la fuente de reloj principal
  SYSTICK_CTRL = 0x07; 

  printf("=== FASE B: Sincronizacion con Semaforos ===\n");
  printf("sem_radio  = Mutex  (init=1)\n");
  printf("sem_energia = Contador (init=3, max 3 LEDs)\n");

  // 6. Bucle Idle (El sistema descansa hasta que SysTick lo despierte)
  while (1) {
    __asm volatile ("wfi"); // Wait For Interrupt (Detiene el reloj del CPU para ahorrar energía)
  }

  return 0;
}
