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
            // Read the GPIO value using the kernel function
            int lectura = k_gpio_get(svc_args[0]);
            // Place the result back in r0 (svc_args[0]) to return to user space
            svc_args[0] = lectura; 
            break;

        case SYS_GPIO_DIR:
            // Validate before executing.
            // Example: Prohibit touching pins 0 and 1 (UART) or pins > 28 (System)
            int pin = svc_args[0];
            if (pin < 2 || pin > 28) {
                svc_args[0] = -1; // Error: Permission denied (EACCES)
            } else {
                // If safe, call the driver
                k_gpio_init(pin, svc_args[1]);
                svc_args[0] = 0; // Success
            }
            break;

        default:
            // Invalid syscall ID, return an error code (e.g., -1)
            svc_args[0] = -1; 
            break;
    }
}