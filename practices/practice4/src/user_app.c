#include <stdint.h>
#include "syscalls.h"

/* Pin Definitions */
#define LED_0 5
#define LED_1 8
#define LED_2 10
#define LED_3 13
#define LED_4 15

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

/**
 * @brief Binary counter task that updates LEDs.
 */
void task_counter(void)
{
  setup_leds();

  // This variable retains its value across context switches
  // because it lives on this task's stack.

  for (uint32_t count = 0; count < 32; count++)
    {
      sys_gpio_set(LED_0, (count >> 0) & 1);
      sys_gpio_set(LED_1, (count >> 1) & 1);
      sys_gpio_set(LED_2, (count >> 2) & 1);
      sys_gpio_set(LED_3, (count >> 3) & 1);
      sys_gpio_set(LED_4, (count >> 4) & 1);

      delay_cycles_exact(41700000 / 4);
    }
    
  // After counting to 31, the task can terminate itself.
  sys_exit();
}

/**
 * @brief Ping-pong LED animation task.
 */
void task_animation(void)
{
  setup_leds();

  // Persistent state variables on the stack
  int pins[] = {LED_0, LED_1, LED_2, LED_3, LED_4};
  int active_led = 0;
  int direction = 1;

  while (1)
    {
      // Turn off all
      for (int i = 0; i < 5; i++)
        {
	  sys_gpio_set(pins[i], 0);
        }

      // Turn on current
      sys_gpio_set(pins[active_led], 1);

      // Compute next position
      active_led += direction;

      if (active_led >= 4)
        {
	  direction = -1;
        }
      else if (active_led <= 0)
        {
	  direction = 1;
        }

      delay_cycles_exact(41700000 / 20);
    }
}

/**
 * @brief Infinite task that rapidly blinks LED_1.
 */
void task3_infinite(void)
{
  sys_gpio_dir(LED_1, 1);
  while (1)
    {
      sys_gpio_set(LED_1, 1);
      delay_cycles_exact(41700000 / 40);
      sys_gpio_set(LED_1, 0);
      delay_cycles_exact(41700000 / 40);
    }
}

/**
 * @brief Finite task that blinks an LED 5 times and then terminates.
 */
void task4_finite(void)
{
  sys_gpio_dir(LED_3, 1);
  
  for (int i = 0; i < 15; i++)
    {
      sys_gpio_set(LED_3, 1);
      delay_cycles_exact(41700000 / 10);
      sys_gpio_set(LED_3, 0);
      delay_cycles_exact(41700000 / 10);
    }
  //Life cycle completed, then we notify the Kernel this task finished
  sys_exit();
}

/**
 * @brief User application entry point (unused).
 */
void user_app_entry(void)
{
  // Unused, tasks start directly
}
