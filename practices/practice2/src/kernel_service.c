#include <stdint.h>

/* Syscall ID for GPIO set operation*/
#define SYS_GPIO_SET 1
/* Syscall ID for GPIO get operation*/
#define SYS_GPIO_GET 2
/* Syscall ID for GPIO direction configuration */
#define SYS_GPIO_DIR 3

// Declarations of kernel-level GPIO functions (defined in kernel_driver.c)
extern void k_gpio_set(uint32_t pin, uint32_t value);
extern int k_gpio_get(uint32_t pin);
extern void k_gpio_init(uint32_t pin, uint32_t output);

/*
 * @brief Kernel service handler for system calls.
 * @param svc_args: Pointer to the user stack frame.
 * @param syscall_id: The value of r7 passed by the assembler
 */
void kernel_service(uint32_t *svc_args, uint32_t syscall_id) {
  switch (syscall_id) {
  case SYS_GPIO_SET:
    // Set the GPIO value using the kernel function
    k_gpio_set(svc_args[0], svc_args[1]);
    break;
    
  case SYS_GPIO_GET:
    //Read the pin state via the hardware driver and write the result back into svc_args[0]
    //Because svc_args points to r0 on the user stack,
    //   this is equivalent to returning the value directly into r0
    svc_args[0] = (uint32_t)k_gpio_get(svc_args[0]);
    break;
    
  case SYS_GPIO_DIR:
    // Validate before executing.
    // Example: Prohibit touching pins 0 and 1 (UART) or pins > 28 (System)
    int pin = svc_args[0];
    if (pin < 2 || pin > 28) {
      svc_args[0] = -1; // Error: Permission denied (EACCES)
    } else {
      //Pin is within the safe range
      //Initialize direction via the driver and return 0 as signal success back to user space through r0
      k_gpio_init(svc_args[0], svc_args[1]);
      svc_args[0] = 0; //success
    }
    break;
    
  default:
    // Invalid syscall ID, return an error code (e.g., -1)
    svc_args[0] = -1; 
    break;
  }
}
