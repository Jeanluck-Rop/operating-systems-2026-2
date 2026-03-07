.syntax unified
.cpu cortex-m0plus
.thumb

.global PendSV_Handler
.type PendSV_Handler, %function

PendSV_Handler:
    /* Context Switching Routine for PendSV Exception */
    
    /* Disable interrupts to ensure atomicity during context switch */
    cpsid i

    /* Mov PSP (Process Stack Pointer) to R0 for manipulation */
    mrs r0, psp

    /* Hardware automatically saves R0-R3, R12, LR, PC, and xPSR on the stack when an exception occurs.  We need to save the remaining registers (R4-R11) manually. */

    /* For 8 registers (R4-R11), we need 32 bytes (8 registers * 4 bytes each) */
    subs r0, #32
    
    /* Store R4-R7 in the lower part of the stack frame */
    stmia r0!, {r4-r7}
    
    /* Move R8-R11 to R4-R7 temporarily to store them */
    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11
    
    /* Store R8-R11 in the upper part of the stack frame (offset +16) */
    stmia r0!, {r4-r7}
    
    /* Restore R0 to point to the start of the full stack frame (including hardware-saved registers) */
    subs r0, #32

    /* Read current_task_id to determine where to save the current context */
    ldr r2, =current_task_id
    /* r3 now holds the current task ID */
    ldr r3, [r2]  
    
    /* If current_task_id is 0, we just switched from task 0 to task 1, so we save the context of task 0 in sp_task1. If it's 1, we save in sp_task2. */
    cmp r3, #0
    beq save_task1   /* Task 0 is running, then we save to sp_task1 */
    cmp r3, #1       /* Is task 1 is running? */
    beq save_task2   /* Yes: save to sp_task2 */
    b save_task3     /* Otherwise: task 2 is running and save to sp_task3 */

save_task3:
    /* Save context of task 2 and prepare task 0 as next (Round-Robin wrap) */
    ldr r1, =sp_task3
    str r0, [r1]            /* Update sp_task3 with the current stack pointer */
    movs r3, #0             /* Prepare the ID for the next task (0) */
    b switch_context        /* Jump to the context switch part */
	
save_task2:
    ldr r1, =sp_task2
    str r0, [r1]            /* Update sp_task2 with the current stack pointer of task 2 */
    movs r3, #2             /* Prepare the ID for the next task (2) MINE */
    b switch_context        /* Jump to the context switch part */

save_task1:
    ldr r1, =sp_task1
    str r0, [r1]            /* Update sp_task1 with the current stack pointer of task 1 */
    movs r3, #1             /* Prepare the ID for the next task (1) */

switch_context:
    str r3, [r2]            /* Store the new current_task_id (the task we are switching to) */

    /* If current_task_id was 0, we need to load task 1's context. If it was 1, we load task 2's context. */
    cmp r3, #0
    beq load_task1   /* New task is 0, then load sp_task1 */
    cmp r3, #1       /* Is new task is 1? */
    beq load_task2   /* Yes: load sp_task2 */
    b   load_task3   /* Otherwise: new task is 2 and load sp_task3 */

load_task3:
    /* Load stack pointer of Task 2 */
    ldr r1, =sp_task3
    ldr r0, [r1]            /* store in r0 the sp_task3 value, which is the stack pointer of task 3 */
    b restore_context
	
load_task2:
    ldr r1, =sp_task2
    ldr r0, [r1]            /* store in r0 the sp_task2 value, which is the stack pointer of task 2 */
    b restore_context	

load_task1:
    ldr r1, =sp_task1
    ldr r0, [r1]            /* store in r0 the sp_task1 value, which is the stack pointer of task 1 */

restore_context:
    /* R0 points to the stack frame of the task we are switching to. We need to restore R4-R11 from this stack frame. */
    /* Recover R8-R11 registers first (they are in the upper part of the stack frame) */
    adds r0, #16
    ldmia r0!, {r4-r7}
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7
    
    /* Recover R4-R7 registers (they are in the lower part of the stack frame) */
    subs r0, #32            /* Get back to the start of the stack frame */
    ldmia r0!, {r4-r7}      /* Restore R4-R7 from the stack frame */
    
    /* At this point, R0-R3, R12, LR, PC, and xPSR are still on the stack and will be automatically restored when we exit the exception handler. We just need to update the PSP to point to the new stack frame for the task we are switching to. */
    adds r0, #16

    /* Update PSP to point to the new stack frame */
    msr psp, r0

    /* Enable interrupts again before exiting the exception handler */
    cpsie i

    /* Special return from exception to switch to the new task. The value 0xFFFFFFFD tells the processor to return to Thread mode and use the Process Stack Pointer (PSP) for the new task. */
    ldr r0, =0xFFFFFFFD
    bx r0

.align 4
