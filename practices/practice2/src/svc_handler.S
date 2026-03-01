.syntax unified
.cpu cortex-m0plus
.thumb

@ Unique SVC handler wrapper to interface with C code.
.global wrapper_svc
.type wrapper_svc, %function

wrapper_svc:
    mrs r0, MSP         @ Arg0 for C: Stack Pointer
    mov r1, r7          @ Arg1 for C: ID Syscall
    
    push {r4, lr}       @ Align stack
    bl kernel_service   @ Jump to C
    pop {r4, pc}        @ Return