#include <stdint.h>
#include "syscalls.h"

/* Pin Definitions */
#define LED_0 7
#define LED_1 9
#define LED_2 10
#define LED_3 12
#define LED_4 13


/**
 * @brief Busy-wait for an exact number of core cycles.
 * @param cycles Number of cycles to delay.
 */
static inline void delay_cycles_exact(uint32_t cycles) {
    __asm volatile (
        "1: \n"
        "sub %[c], %[c], #1 \n"  // 1 cycle
        "bne 1b \n"               // 1-3 cycles (depends on branch)
        : [c] "+r" (cycles)
        :
        : "cc"
    );
}


/**
 * @brief Configure LED GPIOs as outputs.
 */
void setup_leds(void) {
    sys_gpio_dir(LED_0, 1);
    sys_gpio_dir(LED_1, 1);
    sys_gpio_dir(LED_2, 1);
    sys_gpio_dir(LED_3, 1);
    sys_gpio_dir(LED_4, 1);
}

/**
 * @brief Binary counter task that updates LEDs.
 */
void task_counter(void) {
    setup_leds();
    
    // This variable retains its value across context switches
    // because it lives on this task's stack.
    uint8_t count = 0; 

    while (1) {
        sys_gpio_set(LED_0, (count >> 0) & 1);
        sys_gpio_set(LED_1, (count >> 1) & 1);
        sys_gpio_set(LED_2, (count >> 2) & 1);
        sys_gpio_set(LED_3, (count >> 3) & 1);
        sys_gpio_set(LED_4, (count >> 4) & 1);

        count++;
        if (count > 31) count = 0;

        delay_cycles_exact(41700000/2);
    }
}

/**
 * @brief Ping-pong LED animation task.
 */
void task_animation(void) {
    setup_leds();

    // Persistent state variables on the stack
    int pins[] = {LED_0, LED_1, LED_2, LED_3, LED_4};
    int active_led = 0;
    int direction = 1;

    while (1) {
        // Turn off all
        for (int i = 0; i < 5; i++) {
            sys_gpio_set(pins[i], 0);
        }

        // Turn on current
        sys_gpio_set(pins[active_led], 1);

        // Compute next position
        active_led += direction;

        if (active_led >= 4) {
            direction = -1;
        } else if (active_led <= 0) {
            direction = 1;
        }

        delay_cycles_exact(41700000);
    }
}

/**
 * @brief Melody of a lullaby using LEDs
 */
void task_lullaby(void) {
    setup_leds();

    // Persistent state variables on the stack
    int pins[] = {LED_0, LED_1, LED_2, LED_3, LED_4};

    while (1) {
      // Turn off all
      for (int i = 0; i < 5; i++)
	sys_gpio_set(pins[i], 0);

      //Countdown
      sys_gpio_set(pins[4], 1);
      sys_gpio_set(pins[0], 1);
      sys_gpio_set(pins[3], 1);
      sys_gpio_set(pins[1], 1);
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(41700000); //3
      sys_gpio_set(pins[4], 0);
      sys_gpio_set(pins[0], 0);
      delay_cycles_exact(41700000); //2
      sys_gpio_set(pins[3], 0);
      sys_gpio_set(pins[1], 0);
      delay_cycles_exact(41700000); //1
      sys_gpio_set(pins[2], 0);

      delay_cycles_exact(20850000); //0

      /* Firts part*/
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //P-A
      sys_gpio_set(pins[4], 1);
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(22000000);
      sys_gpio_set(pins[4], 0);
      sys_gpio_set(pins[1], 0);      
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(14000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //P-A
      sys_gpio_set(pins[4], 1);
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(31275000);//
      sys_gpio_set(pins[4], 0);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(10000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(25000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(5000000);
      sys_gpio_set(pins[2], 0);
      delay_cycles_exact(5000000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(7500000);
      sys_gpio_set(pins[2], 0);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(6000000);
      sys_gpio_set(pins[3], 0);
      delay_cycles_exact(2000000);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(8500000);
      sys_gpio_set(pins[3], 0);
      //P
      sys_gpio_set(pins[4], 1);
      delay_cycles_exact(5000000);
      sys_gpio_set(pins[4], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(6000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(5000000);
      sys_gpio_set(pins[1], 0);
      //M
      sys_gpio_set(pins[0], 1);
      delay_cycles_exact(20000000);
      sys_gpio_set(pins[0], 0);
      //
      delay_cycles_exact(9000000);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(20000000);
      sys_gpio_set(pins[1], 0);
      delay_cycles_exact(5000000);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(34500000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(3500000);
      sys_gpio_set(pins[2], 0);
      delay_cycles_exact(3500000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(1000000);
      sys_gpio_set(pins[2], 0);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(4500000);
      sys_gpio_set(pins[3], 0);
      delay_cycles_exact(4500000);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(10000000);
      sys_gpio_set(pins[3], 0);
      //P
      sys_gpio_set(pins[4], 1);
      delay_cycles_exact(5000000);
      sys_gpio_set(pins[4], 0);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(6000000);
      sys_gpio_set(pins[3], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(6000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(6000000);
      sys_gpio_set(pins[1], 0);
      //M
      sys_gpio_set(pins[0], 1);
      delay_cycles_exact(10000000);
      sys_gpio_set(pins[0], 0);

      /* Second part */
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(20000000);
      sys_gpio_set(pins[2], 0);
      delay_cycles_exact(5000000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //I-M
      sys_gpio_set(pins[3], 1);
      sys_gpio_set(pins[0], 1);
      delay_cycles_exact(31275000);
      sys_gpio_set(pins[3], 0);
      sys_gpio_set(pins[0], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(14000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //I-M
      sys_gpio_set(pins[3], 1);
      sys_gpio_set(pins[0], 1);
      delay_cycles_exact(25000000);//
      sys_gpio_set(pins[3], 0);
      sys_gpio_set(pins[0], 0);
      //
      delay_cycles_exact(9000000);	    
      //P-D
      sys_gpio_set(pins[4], 1);
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(10000000);//
      sys_gpio_set(pins[4], 0);
      sys_gpio_set(pins[2], 0);
      //I-A
      sys_gpio_set(pins[3], 1);
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(25000000);//
      sys_gpio_set(pins[3], 0);
      sys_gpio_set(pins[1], 0);
      //
      delay_cycles_exact(9000000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(5500000);
      sys_gpio_set(pins[2], 0);
      delay_cycles_exact(5500000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(8000000);
      sys_gpio_set(pins[2], 0);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(7000000);
      sys_gpio_set(pins[3], 0);
      //P
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(7000000);
      sys_gpio_set(pins[2], 0);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(5000000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(5000000);
      //M
      sys_gpio_set(pins[0], 1);
      delay_cycles_exact(58000000);
      sys_gpio_set(pins[3], 0);
      sys_gpio_set(pins[2], 0);
      sys_gpio_set(pins[0], 0);
      
      //
      delay_cycles_exact(90000);

      /* Again */
            /* Firts part*/
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //P-A
      sys_gpio_set(pins[4], 1);
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(22000000);
      sys_gpio_set(pins[4], 0);
      sys_gpio_set(pins[1], 0);      
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(14000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //P-A
      sys_gpio_set(pins[4], 1);
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(31275000);//
      sys_gpio_set(pins[4], 0);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(10000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(25000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(5000000);
      sys_gpio_set(pins[2], 0);
      delay_cycles_exact(5000000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(7500000);
      sys_gpio_set(pins[2], 0);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(6000000);
      sys_gpio_set(pins[3], 0);
      delay_cycles_exact(2000000);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(8500000);
      sys_gpio_set(pins[3], 0);
      //P
      sys_gpio_set(pins[4], 1);
      delay_cycles_exact(5000000);
      sys_gpio_set(pins[4], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(6000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(5000000);
      sys_gpio_set(pins[1], 0);
      //M
      sys_gpio_set(pins[0], 1);
      delay_cycles_exact(20000000);
      sys_gpio_set(pins[0], 0);
      //
      delay_cycles_exact(9000000);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(20000000);
      sys_gpio_set(pins[1], 0);
      delay_cycles_exact(5000000);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(34500000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(3500000);
      sys_gpio_set(pins[2], 0);
      delay_cycles_exact(3500000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(1000000);
      sys_gpio_set(pins[2], 0);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(4500000);
      sys_gpio_set(pins[3], 0);
      delay_cycles_exact(4500000);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(10000000);
      sys_gpio_set(pins[3], 0);
      //P
      sys_gpio_set(pins[4], 1);
      delay_cycles_exact(5000000);
      sys_gpio_set(pins[4], 0);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(6000000);
      sys_gpio_set(pins[3], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(6000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(6000000);
      sys_gpio_set(pins[1], 0);
      //M
      sys_gpio_set(pins[0], 1);
      delay_cycles_exact(10000000);
      sys_gpio_set(pins[0], 0);

      /* Second part */
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(20000000);
      sys_gpio_set(pins[2], 0);
      delay_cycles_exact(5000000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //I-M
      sys_gpio_set(pins[3], 1);
      sys_gpio_set(pins[0], 1);
      delay_cycles_exact(31275000);
      sys_gpio_set(pins[3], 0);
      sys_gpio_set(pins[0], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(14000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //A
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[1], 0);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(9000000);
      sys_gpio_set(pins[2], 0);
      //I-M
      sys_gpio_set(pins[3], 1);
      sys_gpio_set(pins[0], 1);
      delay_cycles_exact(25000000);//
      sys_gpio_set(pins[3], 0);
      sys_gpio_set(pins[0], 0);
      //
      delay_cycles_exact(9000000);	    
      //P-D
      sys_gpio_set(pins[4], 1);
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(10000000);//
      sys_gpio_set(pins[4], 0);
      sys_gpio_set(pins[2], 0);
      //I-A
      sys_gpio_set(pins[3], 1);
      sys_gpio_set(pins[1], 1);
      delay_cycles_exact(25000000);//
      sys_gpio_set(pins[3], 0);
      sys_gpio_set(pins[1], 0);
      //
      delay_cycles_exact(9000000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(5500000);
      sys_gpio_set(pins[2], 0);
      delay_cycles_exact(5500000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(8000000);
      sys_gpio_set(pins[2], 0);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(7000000);
      sys_gpio_set(pins[3], 0);
      //P
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(7000000);
      sys_gpio_set(pins[2], 0);
      //I
      sys_gpio_set(pins[3], 1);
      delay_cycles_exact(5000000);
      //D
      sys_gpio_set(pins[2], 1);
      delay_cycles_exact(5000000);
      //M
      sys_gpio_set(pins[0], 1);
      delay_cycles_exact(60000000);
      sys_gpio_set(pins[3], 0);
      sys_gpio_set(pins[2], 0);
      sys_gpio_set(pins[0], 0);
      
      delay_cycles_exact(150000000);
    }
}

/**
 * @brief User application entry point (unused).
 */
void user_app_entry(void) {
    // Unused, tasks start directly
}
