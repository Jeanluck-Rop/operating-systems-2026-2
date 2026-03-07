#include <stdint.h>
#include "hardware/exception.h"
#include "syscalls.h"

/* --- Hardware Configuration --- */
#define BTN_PIN 2
#define LED_DEBUG 25

/* Direct Debug Registers */
#define SIO_GPIO_OUT_XOR (SIO_BASE + 0x01C) 

/* Core Registers */
#define NVIC_ISER       (*(volatile uint32_t *)0xE000E100)
#define SCB_ICSR        (*(volatile uint32_t *)0xE000ED04)
#define SCB_VTOR        (*(volatile uint32_t *)0xE000ED08) 
#define SHPR3           (*(volatile uint32_t *)0xE000ED20) // System Handler Priority Register 3
#define ICSR_PENDSVSET  (1 << 28)

/* --- OS Configuration --- */
#define STACK_SIZE 256

/* 8-byte aligned stacks (ARM AAPCS requirement) */
uint32_t task1_stack[STACK_SIZE] __attribute__((aligned(8)));
uint32_t task2_stack[STACK_SIZE] __attribute__((aligned(8)));
//
uint32_t task3_stack[STACK_SIZE] __attribute__((aligned(8)));

uint32_t *sp_task1;
uint32_t *sp_task2;
//
uint32_t *sp_task3;

volatile uint32_t current_task_id = 0;

/* External Declarations */
extern void wrapper_svc(void);
extern void task_counter(void);
extern void task_animation(void);
//
extern void task_lullaby(void);

extern void k_gpio_init(uint32_t pin, uint32_t output); 
extern void k_gpio_irq_enable(uint32_t pin, uint32_t rising_edge);
extern void k_gpio_irq_clear(uint32_t pin);
extern void k_gpio_pullup(uint32_t pin);
extern uint32_t ram_vector_table[];
// IMPORTANT: Declare the assembly handler so it can be installed
extern void PendSV_Handler(void); 

/* --- Interrupt Handler (Button) --- */
/**
 * @brief GPIO interrupt handler for the button.
 */
void IO_IRQ_BANK0_Handler(void) {
    // Debug: Toggle LED to confirm we enter here
    *(volatile uint32_t *)SIO_GPIO_OUT_XOR = (1 << LED_DEBUG);

    k_gpio_irq_clear(BTN_PIN);      
    SCB_ICSR |= ICSR_PENDSVSET;     // Request context switch
}

/* --- Vector Table Installation --- */
/**
 * @brief Install the interrupt vector table in RAM.
 */
void install_vector_table(void) {
    // Install button handler (IRQ 13)
    ram_vector_table[16 + 13] = (uint32_t)IO_IRQ_BANK0_Handler;
    
    // Install PendSV handler (Exception 14)
    // This is CRITICAL: if missing, PendSV jumps to an infinite Default_Handler.
    ram_vector_table[14] = (uint32_t)PendSV_Handler; 

    // Point the core to the RAM table
    SCB_VTOR = (uint32_t)ram_vector_table; 
}

/* --- Stack Initialization --- */
/**
 * @brief Initialize a task stack with a synthetic exception frame.
 * @param stack_buffer Base of the task stack buffer.
 * @param task_func Entry point for the task.
 * @param sp_ptr Output pointer for the initial stack pointer value.
 */
void task_init_stack(uint32_t *stack_buffer, void (*task_func)(void), uint32_t **sp_ptr) {
    uint32_t *stk = stack_buffer + STACK_SIZE;

    // Hardware frame
    *(--stk) = 0x01000000;          // xPSR (Thumb bit)
    *(--stk) = (uint32_t)task_func; // PC
    *(--stk) = 0xFFFFFFFD;          // LR (Return to Thread/PSP)
    *(--stk) = 0;                   // R12
    *(--stk) = 0;                   // R3
    *(--stk) = 0;                   // R2
    *(--stk) = 0;                   // R1
    *(--stk) = 0;                   // R0

    // Software frame (R4-R11)
    for (int i = 0; i < 8; i++) {
        *(--stk) = 0;
    }

    *sp_ptr = stk;
}

/**
 * @brief Start the first task by restoring its context.
 */
void os_start_first_task(void) {
    __asm volatile (
        "ldr r0, =sp_task1    \n" 
        "ldr r0, [r0]         \n" 
        
        // Manually load software frame (R4-R11)
        "ldmia r0!, {r4-r7}   \n" 
        "mov r8, r4           \n" 
        "mov r9, r5           \n"
        "mov r10, r6          \n"
        "mov r11, r7          \n"
        "ldmia r0!, {r4-r7}   \n" 
        
        // R0 now points to the start of the hardware frame
        "msr psp, r0          \n" 
        
        "movs r0, #2          \n" // Switch to PSP
        "msr control, r0      \n"
        "isb                  \n"
        
        // Simulate exception return (unstack HW frame from PSP)
        "pop {r0-r5}          \n" // Pop R0-LR
        "pop {r6-r7}          \n" // Pop PC, xPSR
        
        "cpsie i              \n" // Enable interrupts
        "bx r6                \n" // Jump to task
    );
}

/**
 * @brief Kernel entry point.
 * @return Never returns.
 */
int main(void) {
    install_vector_table();
    exception_set_exclusive_handler((enum exception_number)11, wrapper_svc);
    
    // Set PendSV priority to lowest (0xFF)
    // Bits 23:16 control PRI_14 (PendSV)
    SHPR3 |= (0xFF << 16);

    // Hardware init
    k_gpio_init(LED_DEBUG, 1); 
    
    k_gpio_init(BTN_PIN, 0);   
    k_gpio_pullup(BTN_PIN);
    k_gpio_irq_enable(BTN_PIN, 0); 
    
    NVIC_ISER = (1 << 13);

    // Initialize tasks
    task_init_stack(task1_stack, task_counter, &sp_task1);
    task_init_stack(task2_stack, task_animation, &sp_task2);
    //
    task_init_stack(task3_stack, task_lullaby, &sp_task3);
    
    os_start_first_task();

    while (1);
}
