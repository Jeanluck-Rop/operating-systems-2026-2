#include <stdio.h>
#include <stdint.h>
#include <setjmp.h>
#include "pico/stdlib.h"
#include "hardware/exception.h"
#include "hardware/watchdog.h"

/* Minimal MPU Driver (Registers) */
#define MPU_CTRL      (*(volatile uint32_t *)0xE000ED94)
#define MPU_RNR       (*(volatile uint32_t *)0xE000ED98)
#define MPU_RBAR      (*(volatile uint32_t *)0xE000ED9C)
#define MPU_RASR      (*(volatile uint32_t *)0xE000EDA0)

#define TRAP_ADDRESS  0x20010000
#define LED_PIN       10

/* Safe Jump Point for Recovery */
jmp_buf recovery_point;

/* Medical Resilience Flags */
volatile bool safe_mode_enabled = false;
volatile uint32_t fault_count = 0;

//CASO 2:
//volatile uint32_t sim_start_time = 0;
//volatile bool     sim_reset_requested = false;

/**
 * @brief Clinical Incident Logger.
 */
void log_incident(uint32_t pc) {
    fault_count++;
    uint32_t time = to_ms_since_boot(get_absolute_time());
    printf("\n[!] =============================================\n");
    printf("[!]   NEXUS MEDICAL: INCIDENT REPORT #%u          \n", fault_count);
    printf("[!]   TIME: %u ms | ATTACKER PC: 0x%08X      \n", time, pc);
    printf("[!]   STATUS: ISOLATING THREAT...                \n");
    printf("[!] =============================================\n");
}

/**
 * @brief THE RESILIENCE HUB (Revised Lab Template).
 */
void recovery_landing_zone(void) {
    gpio_put(LED_PIN, 1); // Clinical Fault LED
    
    // -------------------------------------------------------------------------
    // REFERENCE EXAMPLE: CHECKPOINT & ROLLBACK 
    // -------------------------------------------------------------------------
    // This is the chosen "warm reset" method. It restores the system without
    // losing the USB connection or resetting the hardware registers.
    //printf("[RECOVERY: ROLLBACK] Context restored to last stable state.\n");
    //printf("[RECOVERY] USB Connection maintained. Resuming monitoring...\n");

    //CASO 1:
    printf("[RECOVERY: DEGRADATION] Switching to safe clinical mode...\n");
    printf("[RECOVERY] Non-critical functions disabled. Core monitoring active.\n");

    //CASO 2:
    //printf("[RECOVERY: RESPAWN] Fault detected. Re-initializing monitor task instance...\n");
    //Reiniciamos el reloj de inicio de la simulacion al momento actual
    //sim_start_time = to_ms_since_boot(get_absolute_time());
    //Indicamos a main que debe usar sim_start_time en lugar de su variable local
    //sim_reset_requested = true;

    //CASO 3:
    //printf("[RECOVERY: FAIL-SAFE] Critical failure detected.\n");
    //printf("[RECOVERY: FAIL-SAFE] State is unrecoverable. Performing hard reboot...\n");
    //printf("[RECOVERY: FAIL-SAFE] USB will disconnect. System restarting...\n");
    //stdio_flush();

    //CASO1:
    //Activamos la bandera de modo seguro
    safe_mode_enabled = true;
        
    // Mandatory Alert Period (so students see the RED led)
    for(volatile uint32_t i=0; i<8000000; i++) { __asm volatile ("nop"); }

    //CASO 0:
    // PERFORM ROLLBACK (Returns execution to setjmp point in main)
    //longjmp(recovery_point, 1);

    // --- EVERYTHING BELOW THIS LINE IS UNREACHABLE UNLESS LONGJMP IS REMOVED ---
    
    // -------------------------------------------------------------------------
    // CHALLENGE 1: GRACEFUL DEGRADATION
    // -------------------------------------------------------------------------
    // [STUDENT_TODO]: Activate safe_mode_enabled = true;
    //
    printf("[RECOVERY: DEGRADATION] Switching to safe clinical mode...\n");
    longjmp(recovery_point, 1);
    //

    // -------------------------------------------------------------------------
    // CHALLENGE 2: TASK RESPAWNING (Simulation)
    // -------------------------------------------------------------------------
    // [STUDENT_TODO]: Reset simulation variables or re-initialize logic.
    //printf("[RECOVERY: RESPAWN] Task re-initialized. Resuming heartbeat...\n");
    //longjmp(recovery_point, 1);

    // -------------------------------------------------------------------------
    // CHALLENGE 3: FAIL-SAFE (HARD RESET)
    // -------------------------------------------------------------------------
    // [STUDENT_TODO]: Implement the hardware reboot using the watchdog.
    // WARNING: This will cause the USB/Putty to disconnect!
    // Fuerza un reinicio de hardware inmediato a traves del watchdog.
    // Los tres parametros son: scratch registers (0,0) y delay en ms (0 = inmediato).
    //watchdog_reboot(0, 0, 0);
    // Esta linea nunca se ejecuta, pero evita warnings del compilador
    //while (1) { __asm volatile ("nop"); }
}

/**
 * @brief C-part of the fault handler.
 */
void HardFault_Handler_C(uint32_t *stack_frame) {
    log_incident(stack_frame[6]);
    stack_frame[6] = (uint32_t)recovery_landing_zone; 
}

/**
 * @brief Assembly entry for HardFault (Fixed Alignment).
 */
void __attribute__((naked)) HardFault_Handler(void) {
    __asm volatile (
        "push {r4, lr} \n"
        "mov r0, lr \n"
        "mov r1, #4 \n"
        "tst r0, r1 \n"
        "beq use_msp \n"
        "mrs r0, psp \n"
        "b call_c \n"
        "use_msp: \n"
        "mrs r0, msp \n"
        "add r0, #8 \n"
        "call_c: \n"
        "bl HardFault_Handler_C \n"
        "pop {r4} \n"
        "pop {r0} \n"
        "mov lr, r0 \n"
        "bx lr \n"
    );
}

void setup_mpu_trap(void) {
    MPU_RNR = 0;
    MPU_RBAR = (TRAP_ADDRESS & 0xFFFFFFE0);
    MPU_RASR = (7 << 1) | (1 << 0) | (0 << 24) | (1 << 16) | (1 << 17);
    MPU_CTRL = (1 << 0) | (1 << 2); 
    __asm volatile ("dsb");
    __asm volatile ("isb");
}

int main() {
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    exception_set_exclusive_handler((enum exception_number)3, HardFault_Handler);
    
    setup_mpu_trap();

    //CASO 3:
    // Al arrancar, verificamos si venimos de un reset por watchdog
    //if (watchdog_caused_reboot()) {
    //    printf("\n[SYSTEM] *** REBOOT DETECTED: System recovered from watchdog reset ***\n");
    //}

    printf("\n=============================================\n");
    printf("   NEXUS MEDICAL: RESILIENCE LABORATORY      \n");
    printf("=============================================\n");

    while(1) {
        // [CHECKPOINT] System stays here after rollback
        if (setjmp(recovery_point) != 0) {
	  //CASO 0/1/3:
	  //Recovery logic after rollback can go here
	  printf("\n[SYSTEM] Recovery complete. Successfuly restored to last stable state.\n");
	  //CASO 2:
	  //printf("\n[SYSTEM] Task respawn complete. System alive after fault.\n");
	}

	//CASO 0/1/3:
        uint32_t start_time = to_ms_since_boot(get_absolute_time());
	
	//CASO 2:
	//Si el respawn pidio reiniciar, usamos su timestamp; si no, el actual
        //uint32_t start_time = sim_reset_requested ? sim_start_time : to_ms_since_boot(get_absolute_time());
        //sim_reset_requested = false;
	
        bool state = false;

        printf("\n[SYSTEM] Resuming heartbeat...\n");

        while(1) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            
            uint32_t blink_speed = safe_mode_enabled ? 1000 : 200;

            static uint32_t last_blink = 0;
            if (now - last_blink > blink_speed) {
                state = !state;
                gpio_put(LED_PIN, state);
                last_blink = now;
            }

            if (now - start_time > 15000) {
                printf("[!] TASK: Attempting critical register write...\n");
                stdio_flush();
                *(volatile uint32_t *)TRAP_ADDRESS = 0xBAD;
            }
        }
    }
}
