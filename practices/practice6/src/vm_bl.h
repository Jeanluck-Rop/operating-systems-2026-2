#ifndef VM_BL_H
#define VM_BL_H

#include "ram.h"

int base_limit(int pid, int address);
void ram_write(int pid, int address, unsigned char value);
unsigned char ram_read(int pid, int address);

// Helper to register a process for simulation
void assign_process(int pid, int base, int limit);

#endif // VM_BL_H
