#include <stdio.h>
#include "vm_bl.h"

#define MAX_PROCESSES 10

process_t processes[MAX_PROCESSES];
int process_count = 0;

/* Registramos un nuevo proceso con su rango de memoria */
void
assign_process(int pid,
	       int base,
	       int limit)
{
  if (process_count < MAX_PROCESSES) {
    processes[process_count].pid = pid;
    processes[process_count].base = base;
    processes[process_count].limit = limit;
    process_count++;
  } else
    fprintf(stderr, "Error: Maximum number of processes reached\n");
}

/* Verificamos si un acceso a memoria es valido para un proceso */
int
base_limit(int pid,
	   int address)
{
  //Buscar el proceso por PID
  for (int i = 0; i < process_count; i++) {
    if (processes[i].pid == pid) {
      //Verificar si la direccion esta dentro del rango [base, base + limite)
      if (address >= processes[i].base && address < processes[i].base + processes[i].limit)
        return 1; //Acceso valido
      else
        return 0; //Violacion de limite
    }
  }
  return -1; //El proceso no existe
}

/*
 * Escribe un byte en RAM previa verificacion Base-Limit,
 * solo se ejecuta la escritura si base_limit valida el acceso
 */
void
ram_write(int pid,
	  int address,
	  unsigned char value)
{
  int result = base_limit(pid, address);
  if (result == 1) {
    ram[address] = value; //Escritura directa en la RAM fisica
    printf("WRITE: Process %d wrote %02X to 0x%03X\n", pid, value, address);
  } else if (result == 0)
    printf("ERROR: Process %d access violation at 0x%03X (Base-Limit)\n", pid, address);
  else
    printf("ERROR: Process %d not found\n", pid);
}

/* Lee un byte de RAM previa verificacion Base-Limite */
unsigned char
ram_read(int pid,
	 int address)
{
  int result = base_limit(pid, address);
  if (result == 1) {
    unsigned char value = ram[address]; //Lectura directa de la RAM fisica
    printf("READ: Process %d read %02X from 0x%03X\n", pid, value, address);
    return value;
  } else if (result == 0)
    printf("ERROR: Process %d access violation at 0x%03X (Base-Limit)\n", pid, address);
  else
    printf("ERROR: Process %d not found\n", pid);
  return 0;
}
