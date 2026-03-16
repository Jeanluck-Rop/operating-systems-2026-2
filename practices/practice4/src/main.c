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

// Prototipos de las tareas de usuario (definidas en user_app.c)
extern void task_counter(void);
extern void task_animation(void);
extern void task3_infinite(void);
extern void task4_finite(void);

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

  // 4. Registrar tareas en el Scheduler (inician en estado DORMANT, esperando al usuario)
  task_create(0, task_counter);
  task_create(1, task_animation);
  task_create(2, task3_infinite);
  task_create(3, task4_finite);

  // 5. Configurar e iniciar SysTick
  // Para un reloj de 125MHz, contar 1,250,000 ciclos equivale a 10 milisegundos
  SYSTICK_LOAD = 1250000 - 1; 
  SYSTICK_VAL = 0;
  // 0x07 = Habilita el Timer, habilita la Interrupción, y usa la fuente de reloj principal
  SYSTICK_CTRL = 0x07; 

  printf("Kernel de la Practica 4 Iniciado.\n");
  printf("Envia 1, 2, 3 o 4 por la terminal USB para encolar tareas.\n");

  // 6. Bucle Idle (El sistema descansa hasta que SysTick lo despierte)
  while (1) {
    __asm volatile ("wfi"); // Wait For Interrupt (Detiene el reloj del CPU para ahorrar energía)
  }

  return 0;
}

