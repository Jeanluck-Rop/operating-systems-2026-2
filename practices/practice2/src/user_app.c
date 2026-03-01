#include <stdint.h>
#include "syscalls.h"

/* GPIO LED pin definition */
//#define LED_PIN 25
#define LED_PIN 13
/* GPIO Button pin definition */
//#define BTN_PIN 15
#define BTN_PIN 2

/* Funcion auxiliar para dar un retraso (o delay) forzado entre las acciones de prender y apagar el LED */
void
sys_delay(uint32_t time)
{
  for (volatile uint32_t i = 0; i <= time; i ++);
}

int user_app_entry(void) {
  // Setup Phase (Unprivileged): The user program configures its resources using syscalls.

  // TODO: Configure LED_PIN as output and BTN_PIN as input using sys_gpio_dir syscall
  sys_gpio_dir(LED_PIN, 1);
  sys_gpio_dir(BTN_PIN, 0);  
  int LED_STATE = 0;
  int BTN_STATE = 0;
  int P_BTN_STATE = 1;
  
  while (1) {
    // Operational Phase (Unprivileged): The user program performs its main functionality.
    // TODO: Read the button state using sys_gpio_get 
    // set the LED state accordingly using sys_gpio_set
    BTN_STATE = sys_gpio_get(BTN_PIN);
    if (P_BTN_STATE == 1 && BTN_STATE == 0) {
      sys_delay(500000);
      if (sys_gpio_get(BTN_PIN) == 0) {
        LED_STATE = !LED_STATE;
	sys_gpio_set(LED_PIN, LED_STATE);
      }
    }
    P_BTN_STATE = BTN_STATE;
  }
  
  // Should never reach here
  return 0;
}
