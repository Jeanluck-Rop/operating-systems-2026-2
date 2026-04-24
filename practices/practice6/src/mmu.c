#include <stdio.h>
#include "mmu.h"
#include "tlb.h"

#define PAGE_TABLE_SIZE 256
//Tabla de paginas con la que mapeamos VPN -> PFN
int page_table[PAGE_TABLE_SIZE];
int use_tlb = 0; //Flag to enable TLB behavior

//Inicializa la tabla de paginas (todas las entradas invalidas) y el TLB
void
init_mmu()
{
  for (int i = 0; i < PAGE_TABLE_SIZE; i++)
    page_table[i] = -1;
  init_tlb();
}

//Activa el uso del TLB como cache de traducciones
void
enable_tlb()
{
  use_tlb = 1;
}

/* Establece un mapeo pagina -> marco en la tabla */
void
set_page_table_entry(int page,
		     int frame)
{
  if (page >= 0 && page < PAGE_TABLE_SIZE) {
    page_table[page] = frame;
    printf("MMU: Page %d mapped to Frame %d\n", page, frame);
  } else
    fprintf(stderr, "Error: Page index out of bounds\n");
}

/*
 * Traduce una direccion virtual a direccion fisica
 * Procedimiento:
 *   1. Calcula VPN = virtual_addr / page_size
 *   2. Calcula offset = virtual_addr % page_size
 *   3. Si el TLB esta activo, busca ahi primero (rapido, 1 ciclo)
 *   4. Si es TLB miss, consulta la tabla de paginas (Page Table Walk)
 *   5. Si se encontro en tabla y TLB activo, inserta la traduccion en TLB
 *   6. Direccion fisica = frame * page_size + offset
 */
int
mmu_translate(int virtual_addr,
	      int page_size)
{
  if (page_size <= 0)
    return -1;
  int page = virtual_addr / page_size;   //Numero de pagina virtual
  int offset = virtual_addr % page_size; //Desplazamiento dentro de la pagina
  int frame = -1;
  
  //Paso 1: Consultar TLB si esta habilitado
  if(use_tlb)
    frame = tlb_lookup(page);
  //Paso 2: Si TLB miss (o TLB deshabilitado), recorrer tabla de paginas
  if (frame == -1) {
    if (page >= 0 && page < PAGE_TABLE_SIZE)
      frame = page_table[page];
    if (frame != -1 && use_tlb)
      tlb_insert(page, frame);
  }

  if (frame != -1) {
    //Calculamos direccion fisica, base del marco + offset
    int physical_addr = frame * page_size + offset;
    printf("TRANSLATE: Virtual 0x%04X -> Physical 0x%04X (Frame %d)\n", 
	   virtual_addr, physical_addr, frame);
    return physical_addr;
  } else {
    printf("TRANSLATE ERROR: Page %d is not mapped\n", page);
    return -1;
  }
}
