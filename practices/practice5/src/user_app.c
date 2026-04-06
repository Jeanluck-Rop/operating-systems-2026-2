#include <stdint.h>
#include <stdio.h>
#include "syscalls.h"
#include "scheduler.h"

/* Pin Definitions */
#define LED_0 5
#define LED_1 8
#define LED_2 10
#define LED_3 13
#define LED_4 15

#define DELAY_500MS 20850000

/* ------------------------------------------------------------------
 * Semáforos definidos en main.c
 * ------------------------------------------------------------------ */
extern semaphore_t sem_radio;   // Mutex  (init = 1): acceso exclusivo al UART
extern semaphore_t sem_energia; // Contador (init = 3): máx. 3 LEDs simultáneos

/**
 * @brief Busy-wait for an exact number of core cycles.
 * @param cycles Number of cycles to delay.
 */
static inline void delay_cycles_exact(uint32_t cycles)
{
  __asm volatile(
		 "1: \n"
		 "sub %[c], %[c], #1 \n" // 1 cycle
		 "bne 1b \n"             // 1-3 cycles (depends on branch)
		 : [c] "+r"(cycles)
		 :
		 : "cc");
}

/**
 * @brief Configure LED GPIOs as outputs.
 */
void setup_leds(void)
{
  sys_gpio_dir(LED_0, 1);
  sys_gpio_dir(LED_1, 1);
  sys_gpio_dir(LED_2, 1);
  sys_gpio_dir(LED_3, 1);
  sys_gpio_dir(LED_4, 1);
}

/* */
void
task_subsystem1(void)
{
  sys_gpio_dir(LED_0, 1);

  while (1) {
    sys_semaphore_wait(&sem_energia); //acquire energy slot (max 3)
    
    sys_gpio_set(LED_0, 1);
    delay_cycles_exact(DELAY_500MS);
    sys_gpio_set(LED_0, 0);

    sys_semaphore_post(&sem_energia); //release energy slot
 
    sys_semaphore_wait(&sem_radio); //acquire UART channel (mutex)

    printf("[1st SUBSYSTEM]: Calentamiento finalizado. Operacion nominal alcanzada. Transfiriendo paquetes de telemetria y reportando estado actual a base terrestre...\n");
    
    sys_semaphore_post(&sem_radio); //release UART channel
  }
}

/* */
void
task_subsystem2(void)
{
  sys_gpio_dir(LED_1, 1);

  while (1) {
    sys_semaphore_wait(&sem_energia); //acquire energy slot (max 3)
	
    sys_gpio_set(LED_1, 1);
    delay_cycles_exact(DELAY_500MS);
    sys_gpio_set(LED_1, 0);

    sys_semaphore_post(&sem_energia); //release energy slot

    sys_semaphore_wait(&sem_radio); //acquire UART channel (mutex)
	
    printf("[2nd SUBSYSTEM]: Calentamiento finalizado. Operacion nominal alcanzada. Transfiriendo paquetes de telemetria y reportando estado actual a base terrestre...\n");
    
    sys_semaphore_post(&sem_radio); //release UART channel
  }
}

/* */
void
task_subsystem3(void)
{
  sys_gpio_dir(LED_2, 1);

  while (1) {
    sys_semaphore_wait(&sem_energia); //acquire energy slot (max 3)
	
    sys_gpio_set(LED_2, 1);
    delay_cycles_exact(DELAY_500MS);
    sys_gpio_set(LED_2, 0);

    sys_semaphore_post(&sem_energia); //release energy slot

    sys_semaphore_wait(&sem_radio); //acquire UART channel (mutex)
	
    printf("[3rd SUBSYSTEM]: Calentamiento finalizado. Operacion nominal alcanzada. Transfiriendo paquetes de telemetria y reportando estado actual a base terrestre...\n");
    
    sys_semaphore_post(&sem_radio); //release UART channel
  }
}

/* */
void
task_subsystem4(void)
{
  sys_gpio_dir(LED_3, 1);

  while (1) {
    sys_semaphore_wait(&sem_energia); //acquire energy slot (max 3)
	
    sys_gpio_set(LED_3, 1);
    delay_cycles_exact(DELAY_500MS);
    sys_gpio_set(LED_3, 0);

    sys_semaphore_post(&sem_energia); //release energy slot

    sys_semaphore_wait(&sem_radio); //acquire UART channel (mutex)
	
    printf("[4th SUBSYSTEM]: Calentamiento finalizado. Operacion nominal alcanzada. Transfiriendo paquetes de telemetria y reportando estado actual a base terrestre...\n");
    
    sys_semaphore_post(&sem_radio); //release UART channel
  }
}

/* */
void
task_subsystem5(void)
{
  sys_gpio_dir(LED_4, 1);

  while (1) {
    sys_semaphore_wait(&sem_energia); //acquire energy slot (max 3)
	
    sys_gpio_set(LED_4, 1);
    delay_cycles_exact(DELAY_500MS);
    sys_gpio_set(LED_4, 0);

    sys_semaphore_post(&sem_energia); //release energy slot

    sys_semaphore_wait(&sem_radio); //acquire UART channel (mutex)
	
    printf("[5th SUBSYSTEM]: Calentamiento finalizado. Operacion nominal alcanzada. Transfiriendo paquetes de telemetria y reportando estado actual a base terrestre...\n");
    
    sys_semaphore_post(&sem_radio); //release UART channel
  }
}

/**
 * @brief User application entry point (unused).
 */
void user_app_entry(void) { /* Unused */ }
