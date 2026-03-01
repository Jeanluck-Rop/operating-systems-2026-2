#include <stdint.h>
#include "hardware/exception.h"

/* Import the svc wrapper from asm */
extern void wrapper_svc(void);

/* Import the User Entry Point */
extern void user_app_entry(void);

int main()
{
    // Indicate that the system is using the SVC handler for system calls.
    exception_set_exclusive_handler((enum exception_number)11, wrapper_svc);

    // USER LAUNCH
    // This is where the OS "hands over" control.
    // In future practices (Scheduler), the Timer would be configured here.
    user_app_entry();

    // KERNEL PANIC
    // A user program should never return (it should be an infinite loop).
    // If it gets here, the system stops for safety.
    while (1);

    // Should never reach here
    return 0;
}
