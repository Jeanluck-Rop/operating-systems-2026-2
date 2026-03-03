#include <stdint.h>
#include "syscalls.h"

/* GPIO LED pin definition */
#define LED_PIN 13
/* GPIO Button pin definition */
#define BTN_PIN 2

/* Auxiliary function that performs a busy-wait delay  */
void sys_delay(uint32_t time) {
  for (volatile uint32_t i = 0; i <= time; i ++);
}

int user_app_entry(void) {
  // Setup Phase (Unprivileged): The user program configures its resources using syscalls.

  //Configure LED_PIN as output (1) and BTN_PIN as input (0)
  //Neither call touches hardware directly, both go through the
  //  svc mechanism so the kernel can validate and execute them
  sys_gpio_dir(LED_PIN, 1);
  sys_gpio_dir(BTN_PIN, 0);  
  int LED_STATE = 0; //track the current state of the LED
  int BTN_STATE = 0; //hold the current button reading from the syscall
  
  //hold the previous button state
  //Initialized to 1, because the pull-up resistor holds the pin high at rest
  int P_BTN_STATE = 1;
  
  while (1) {
    // Operational Phase (Unprivileged): The user program performs its main functionality.

    //Read the current button state via syscall.
    //Kernel reads the SIO_GPIO_IN register and returns the corresponding bitx
    BTN_STATE = sys_gpio_get(BTN_PIN);

    //Falling edge detection:
    //  the button was released (1) last iteration and is now pressed (0) this iteration
    //This executes exactly once per physical press, not continuously while held down
    if (P_BTN_STATE == 1 && BTN_STATE == 0) {
      //Software debounce: wait and re-read the pin
      //Mechanical buttons bounce between 0 and 1 for a few milliseconds after being pressed
      //If the pin is still 0/1 after the delay, the press is genuine and not electrical noise
      sys_delay(500000);
      if (sys_gpio_get(BTN_PIN) == 0) {
	//Toggle the LED state and apply it via syscall
        //Due to FASE B: LED_STATE is used instead of BTN_STATE because
        //  the LED must remember its own state independently of the button position
        LED_STATE = !LED_STATE;
	sys_gpio_set(LED_PIN, LED_STATE);
      }
    }
    
    //Store the current reading as the previous state for the next iteration
    //This is what makes falling edge detection possible on the next cycle
    P_BTN_STATE = BTN_STATE;
  }
  
  // Should never reach here
  return 0;
}
