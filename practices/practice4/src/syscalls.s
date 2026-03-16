.syntax unified
.cpu cortex-m0plus
.thumb

@ IDs definitions as seen in kernel_driver.c
.equ SYS_GPIO_SET, 1
.equ SYS_GPIO_GET, 2
.equ SYS_GPIO_DIR, 3
.equ SYS_EXIT,     4

@ --- Function: void sys_gpio_dir(int pin, int out) ---
.global sys_gpio_dir
.type sys_gpio_dir, %function
sys_gpio_dir:
    @ r0: pin
    @ r1: out (1 = output, 0 = input)
    @ Save r7 in r12 (caller-saved) to avoid touching the user stack
    mov r12, r7
    movs r7, #SYS_GPIO_DIR
    svc #0
    mov r7, r12
    bx lr

@ --- Function: void sys_gpio_set(int pin, int value) ---
.global sys_gpio_set
.type sys_gpio_set, %function
sys_gpio_set:
    @ r0: pin (Argument 1) - Already in place
    @ r1: value (Argument 2) - Already in place
    @ Save r7 in r12 to avoid modifying user stack
    mov r12, r7
    movs r7, #SYS_GPIO_SET @ Load syscall ID (1)
    svc #0              @ System Call! (Jump to Kernel)
    mov r7, r12         @ Restore r7
    bx lr               @ Return

@ --- Function: int sys_gpio_get(int pin) ---
.global sys_gpio_get
.type sys_gpio_get, %function
sys_gpio_get:
    @ r0: pin (Argument 1)
    @ Save r7 in r12 to avoid modifying user stack
    mov r12, r7
    movs r7, #SYS_GPIO_GET @ Load syscall ID (2)
    svc #0              @ Kernel will place the result in r0
    mov r7, r12
    bx lr

@ --- Function: void sys_exit(void) ---
.global sys_exit
.type sys_exit, %function
sys_exit:
    @ Save r7 in r12 to avoid modifying user stack
    mov r12, r7
    movs r7, #SYS_EXIT  @ Load syscall ID (4)
    svc #0              @ Jump to Kernel
    mov r7, r12         @ Restore r7
    bx lr               @ Return (though we won't actually return)