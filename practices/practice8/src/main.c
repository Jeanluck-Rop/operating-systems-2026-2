#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

// Constantes del FS
#define FS_BASE_OFFSET (1024 * 1024) // Offset de 1 MB (deja 1 MB para el programa)
#define FS_BASE_ADDRESS (XIP_BASE + FS_BASE_OFFSET)
#define MAX_FILES 16
#define MAGIC_NUMBER 0x4649534F // "FISO"

// Tamano de un sector de borrado en la Flash NAND del RP2040: 4096 bytes
// Solo se activa si el SDK no lo define previamente
#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE 4096
#endif

// 8 bytes (cabecera) + 16 * 24 bytes (entradas) = 392 bytes totales
// Necesitamos exactamente 2 paginas (512 bytes) para programarlo por completo
#define META_PROGRAM_SIZE 512

typedef enum {
  STATUS_DELETED = 0x00,
  STATUS_OCCUPIED = 0xAA,
  STATUS_FREE = 0xFF
} FileStatus;

typedef struct {
  char name[12];
  uint32_t offset;
  uint32_t size;
  uint8_t status;
  uint8_t padding[3];
} FileEntry;

typedef struct {
  uint32_t magic_number;
  uint32_t total_size;
  FileEntry entries[MAX_FILES];
} MetaData_Table;

// Apuntador para leer la memoria Flash directamente mapeada (XIP - Execute In Place)
const MetaData_Table* fs_meta = (const MetaData_Table *)(FS_BASE_ADDRESS);


/* Encapsulamos el patron borrar-programar del sector 0 del FS
   Recibimos un apuntador a la copia RAM ya modificada de MetaData_Table
   y la persistimos en la Flash de forma atomica */
static void
fs_flush_meta(const MetaData_Table *m)
{
  //Preparamos buffer de 512 bytes relleno con 0xFF y copiamos la tabla
  uint8_t meta_buf[META_PROGRAM_SIZE];
  memset(meta_buf, 0xFF, META_PROGRAM_SIZE);
  memcpy(meta_buf, m, sizeof(MetaData_Table));
  
  // Borramos el sector 0 del FS y lo reescribimos con los metadatos nuevos
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FS_BASE_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(FS_BASE_OFFSET, meta_buf, META_PROGRAM_SIZE);
  restore_interrupts(ints);
}


/*
 ==========================================
 * MODULO 1: Inicializacion y Superbloque *
 ==========================================
*/

// fs_format se entrega resuelta como ejemplo de uso de la API de Flash
void fs_format () {
  // 1. Deshabilitar interrupciones antes de tocar la Flash
  uint32_t ints = save_and_disable_interrupts();
  
  // 2. Borrar el sector completo (4096 bytes )
  flash_range_erase(FS_BASE_OFFSET, FLASH_SECTOR_SIZE);
  
  // 3. Crear los nuevos metadatos en RAM
  MetaData_Table new_meta;
  memset(&new_meta, 0xFF, sizeof(MetaData_Table));
  new_meta.magic_number = MAGIC_NUMBER;
  new_meta.total_size = 0;

  // 4. Preparar el buffer del taman o exacto (multiplo de 256 bytes)
  uint8_t meta_buf[META_PROGRAM_SIZE];
  memset(meta_buf, 0xFF, META_PROGRAM_SIZE);
  memcpy(meta_buf, &new_meta, sizeof(MetaData_Table));
  
  // 5. Programar la Flash y restaurar interrupciones
  flash_range_program(FS_BASE_OFFSET, meta_buf, META_PROGRAM_SIZE);
  restore_interrupts(ints);
  
  printf("FS:Formateado exitosamente.\n");
}

/* Intentamos montar el FS existente leyendo la firma magica,
   si la firma no coincide, fuerza un formateo inicial. */
void
fs_init ()
{
  //Leemos el magic_number directamente desde la ventana XIP (sin copiar a RAM) y
  //  lo comparamos con la constante esperada */
  if (fs_meta->magic_number == MAGIC_NUMBER) //La firma coincide, el FS fue formateado previamente y es valido
    printf("Sistema de archivos PicoFS montado exitosamente!\n Magic Number: 0x%08X\n", fs_meta->magic_number);
  else {
    //La firma no coincide, debemos formatear antes de operar
    printf("Magic Number no coincide (leído: 0x%08X). Sistema no formateado.\n", fs_meta->magic_number);
    printf("Forzando formateo del sistema de archivos...\n");
    fs_format(); //Escribimos el superbloque con la firma correcta
  }
}


/*
  ==================================================
  * MODULO 2: Asignacion de Metadatos y Alineacion *
  ==================================================
*/

/* Registramos un archivo nuevo en la tabla de directorio
   Se retorna el indice asignado, o un codigo de error negativo */
int
fs_create(const char* name)
{
  if (strlen(name) >= 12) {
    printf("FS Error: Nombre muy largo.\n");
    return -1;
  }
  
  /* Recorremos toda la tabla de directorio para:
     a) detectar si ya existe un archivo con ese nombre (colision)
     b) encontrar la primera entrada libre
     c) calcular el offset del siguiente bloque disponible */
  int free_idx = -1; //indice de la primera entrada STATUS_FREE
  /* El primer sector (sector 0 del FS) es el superbloque
     Los archivos empiezan a partir del sector 1, es decir
     FS_BASE_OFFSET + FLASH_SECTOR_SIZE */
  uint32_t max_offset = FS_BASE_OFFSET + FLASH_SECTOR_SIZE;

  for (int i = 0; i < MAX_FILES; i++) {
    if (fs_meta->entries[i].status == STATUS_OCCUPIED) {
      //Colision, ya existe un archivo activo con ese nombre
      if (strncmp(fs_meta->entries[i].name, name, 12) == 0) {
	printf("FS Error: El archivo '%s' ya existe.\n", name);
	return -2;
      }

      /* Calculamos el extremo superior del archivo i y alinearlo al
	 siguiente limite de sector (multiplo de FLASH_SECTOR_SIZE),
	 con esto garantizamos que cada archivo empiece en un sector limpio */
      uint32_t end = fs_meta->entries[i].offset + fs_meta->entries[i].size;
      uint32_t aligned = ((end + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
      
      if (aligned > max_offset)
	max_offset = aligned; //Actualizamos el offset maximo encontrado
      
    } else if (fs_meta->entries[i].status == STATUS_FREE && free_idx == -1)
      free_idx = i; //Guardamos solo el primer slot libre encontrado
  }
  
  // Si no se encontro ninguna entrada libre, el directorio esta lleno
  if (free_idx == -1) {
    printf("FS Error: No hay entradas libres en el directorio.\n");
    return -3;
  }
  
  // Copiamos los metadatos actuales a RAM para modificarlos
  // (la Flash XIP es de solo lectura, no se puede escribir directamente)
  MetaData_Table updated_meta;
  memcpy(&updated_meta, fs_meta, sizeof(MetaData_Table));

  //Rellenamos la entrada libre con los datos del nuevo archivo
  strncpy(updated_meta.entries[free_idx].name, name, 12);  //Nombre
  updated_meta.entries[free_idx].offset = max_offset;      //Primer sector disponible
  updated_meta.entries[free_idx].size = 0;                 //Sin datos aun
  updated_meta.entries[free_idx].status = STATUS_OCCUPIED; //Marcamos como ocupado
  
  fs_flush_meta(&updated_meta);
  
  printf("FS: Archivo '%s' creado en el slot %d (offset 0x%08X).\n", name, free_idx, max_offset);
  return free_idx; //Exito, devolvemos el indice asignado
}


/*
  ================================
  *  MODULO 3: Read-Modify-Write *
  ================================
*/

/* Escribimos datos en el sector asignado al archivo y actualizamos
   el campo 'size' en el superbloque de forma persistente */
int
fs_write(const char* name,
	 const uint8_t* data,
	 uint32_t size)
{
  int target_idx = -1;
  
  // Busqueda del archivo
  for (int i = 0; i < MAX_FILES; i++) {
    if (fs_meta -> entries[i].status == STATUS_OCCUPIED && strncmp(fs_meta -> entries[i].name, name, 12) == 0) {
      target_idx = i;
      break;
    }
  }
  
  if (target_idx == -1) {
    printf("FS Error: Archivo ’%s’ no encontrado.\n", name);
    return -1;
  }

  //1. Recuperamos el offset absoluto donde viven los datos del archivo
  uint32_t offset = fs_meta->entries[target_idx].offset;
  
  //2. Calculamos cuantos sectores completos de 4096 bytes se necesitan para
  //    contener 'size' bytes (redondeanod hacia arriba) */
  uint32_t erase_size = ((size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;

  /* ------------------------------------------------------------------
   *  PUNTO EXTRA: Prevencion de Desbordamiento
   *  Recorremos el directorio buscando el archivo OCUPADO cuyo offset
   *  sea el mas chico entre los que sean estrictamente mayores al
   *  offset del archivo destino.
   *  Ese es el vecino inmediato, si no existe ninguno,
   *    next_offset queda en UINT32_MAX (sin limite)
   * ------------------------------------------------------------------ */
  uint32_t next_offset = UINT32_MAX; // Sin vecino activo por defecto
  
  for (int i = 0; i < MAX_FILES; i++) {
    if (i == target_idx) continue; //Saltar el propio archivo destino
    
    // Solo los archivos OCUPADOS representan datos validos que proteger
    if (fs_meta->entries[i].status == STATUS_OCCUPIED) {
      uint32_t candidate = fs_meta->entries[i].offset;
      
      // Candidato valido: offset mayor al del archivo destino
      //  y menor al vecino mas cercano encontrado hasta ahora
      if (candidate > offset && candidate < next_offset) {
	next_offset = candidate;
      }
    }
  }
  
  // Si la region de borrado invade el sector del vecino, abortar
  if (offset + erase_size > next_offset) {
        printf("FS Error: Escritura rechazada. Los %u bytes de '%s' "
	"invadiran el sector del vecino en 0x%08X.\n",
	size, name, next_offset);
        return -2; // Codigo especifico de colision de espacio
  }
  
  /* Calculamos el tamano del buffer de programacion:
     debe ser multiplo de FLASH_PAGE_SIZE (256 bytes), por restriccion de flash_range_program */
  uint32_t program_size = ((size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE)  * FLASH_PAGE_SIZE;
  
  /* Crear el buffer de escritura en la pila, inicializado a 0xFF
     (bits no escritos quedan en su estado por defecto de Flash borrada) */
  uint8_t write_buffer[program_size];
  memset(write_buffer, 0xFF, program_size); //Rellenamos todo con 0xFF
  memcpy(write_buffer, data, size);         //Copiamos solo los bytes reales

  /* 3-5. Ciclo Read-Modify-Write en Flash:
     a) Deshabilitamos interrupciones (la Flash no puede ser interrumpida)
     b) Borramos los sectores necesarios (regresa bits a 1)
     c) Programamos los datos nuevos (solo puede cambiar bits de 1 a 0)
     d) Restauramos interrupciones */
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(offset, erase_size);                   //Borrado fisico del area
  flash_range_program(offset, write_buffer, program_size); //Escritura de datos
  restore_interrupts(ints);
  
  /* 6. Si el tamano cambio respecto a lo almacenado en los metadatos,
     actualizamos el campo 'size' en el superbloque para hacerlo persistente */
  if (fs_meta->entries[target_idx].size != size) {
    MetaData_Table updated_meta;
    memcpy(&updated_meta, fs_meta, sizeof(MetaData_Table)); //Copia en RAM
    updated_meta.entries[target_idx].size = size;           //Actualizamos tamano
    fs_flush_meta(&updated_meta);
  }

  printf("FS: Escritos %u bytes en '%s'.\n", size, name);
  return 0;
}

/* Copiamos los datos del archivo al buffer usando XIP (sin DMA ni SPI)
   Se retorna el numero de bytes leidos, o -1 si el archivo no existe */
int
fs_read(const char* name,
	uint8_t* buffer,
	uint32_t max_size)
{
  int target_idx = -1;
  
  //1-2. Buscamos el archivo, retornamos -1 si no se encuentra
  for (int i = 0; i < MAX_FILES; i++) {
    if (fs_meta->entries[i].status == STATUS_OCCUPIED &&
	strncmp(fs_meta->entries[i].name, name, 12) == 0) {
      target_idx = i;
      break;
    }
  }
  if (target_idx == -1)
    return -1;

  //3. Obtenemos el tamano real registrado en los metadatos
  uint32_t file_size = fs_meta->entries[target_idx].size;
  
  //4. Leemos solo lo que cabe en el buffer del llamador (evitar desbordamiento)
  uint32_t read_size = (file_size < max_size) ? file_size : max_size;
  
  //5. Calcular la direccion de memoria fisica del archivo en la ventana XIP
  //   XIP_BASE (0x10000000) + offset absoluto dentro de la Flash = apuntador directo
  const uint8_t *phys_addr = (const uint8_t *)(XIP_BASE + fs_meta->entries[target_idx].offset);
  
  //6. Copiamos los bytes directamente desde la Flash mapeada al buffer del llamador
  memcpy(buffer, phys_addr, read_size);
  
  //7. Devolvemos el numero de bytes efectivamente leidos
  return (int)read_size;
}


/*
  ============================================
  * MODULO 4: Borrado Logico y Visualizacion *
  ============================================
*/
/* Marcamos el archivo como DELETED en los metadatos sin borrar
   fisicamente sus datos en Flash (borrado logico) */
int
fs_delete(const char* name)
{
  int target_idx = -1;
  
  //1. Localizamos el archivo activo por nombre
  for (int i = 0; i < MAX_FILES; i++) {
    if (fs_meta->entries[i].status == STATUS_OCCUPIED &&
	strncmp(fs_meta->entries[i].name, name, 12) == 0) {
      target_idx = i;
      break;
    }
  }  
  //Si no existe, no hay nada que borrar
  if (target_idx == -1) {
    printf("FS Error: Archivo '%s' no encontrado para borrar.\n", name);
    return -1;
  }
  
  //2. Copiamos el superbloque a RAM para poder modificarlo
  MetaData_Table updated_meta;
  memcpy(&updated_meta, fs_meta, sizeof(MetaData_Table));
  
  /* 3. Cambiamos el estado de la entrada a DELETED (0x00)
     Los datos fisicos del archivo permanecen intactos en su sector,
     solo el metadato de estado cambia (borrado logico) */
  updated_meta.entries[target_idx].status = STATUS_DELETED;
  
  /* 4. Persistimos el superbloque modificado en la Flash
     (solo se reescribe el sector 0 del FS, no el sector de datos) */
  uint8_t meta_buf[META_PROGRAM_SIZE];
  memset(meta_buf, 0xFF, META_PROGRAM_SIZE);
  memcpy(meta_buf, &updated_meta, sizeof(MetaData_Table));
  
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FS_BASE_OFFSET, FLASH_SECTOR_SIZE);             //Borramos sector de metadatos
  flash_range_program(FS_BASE_OFFSET, meta_buf, META_PROGRAM_SIZE); //Reescribimos
  restore_interrupts(ints);

  printf("FS: Archivo '%s' marcado como DELETED (borrado logico).\n", name);
  return 0;
}

/* Imprimimos una tabla diagnostica de todas las entradas no libres,
   mostrando nombre, offset, tamanno y estado de cada archivo */
void
fs_dump()
{
  printf("\n=== Mapa de PicoFS ===\n");
  printf("Magic Number: 0x%08X\n", fs_meta->magic_number);
  printf("%-12s | %-10s | %-8s | %-8s\n", "Name", "Offset", "Size", "Status");
  printf("--------------------------------------------------\n");
  
  //1-2. Recorremos todas las entradas del directorio
  for (int i = 0; i < MAX_FILES; i++) {
    uint8_t status = fs_meta->entries[i].status;
    
    //Omitimos las entradas libres, solo mostrar OCCUPIED y DELETED
    if (status != STATUS_FREE) {
      //3. Convertimos el codigo hexadecimal de estado a cadena legible
      const char *status_str = "UNKNOWN";
      if (status == STATUS_OCCUPIED)
	status_str = "OCCUPIED";
      else if (status == STATUS_DELETED)
	status_str = "DELETED";
      
      //Imprimimo la fila con formato alineado
      printf("%-12.12s | 0x%08X | %-8u | %-8s\n",
	     fs_meta->entries[i].name,
	     fs_meta->entries[i].offset,
	     fs_meta->entries[i].size,
	     status_str);
    }
  }
  
  printf("=======================\n\n");
}


/* =========================================================
 *  PUNTO EXTRA: Compactacion / Garbage Collection
 *
 *  Eliminamos los huecos dejados por archivos DELETED desplazando
 *  fisicamente los datos de los archivos OCCUPIED que esten despues
 *  de cada hueco hacia el espacio libre que quedo antes de ellos,
 *  y actualizando sus offsets en el superbloque.
 *
 *  Algoritmo paso a paso:
 *  1. Ordenar las entradas OCCUPIED por offset de menor a mayor
 *     (burbuja sobre la copia RAM, sin tocar la Flash todavia).
 *  2. Recalcular los offsets contiguos desde el primer sector
 *     de datos (FS_BASE_OFFSET + FLASH_SECTOR_SIZE).
 *  3. Para cada archivo cuyo offset cambio:
 *     a. Leer sus datos desde la direccion XIP antigua.
 *     b. Borrar y escribir en la nueva direccion.
 *     c. Actualizar el offset en la copia RAM del superbloque.
 *  4. Marcar todas las entradas DELETED como STATUS_FREE para
 *     liberar sus slots en el directorio.
 *  5. Persistir el superbloque compactado en la Flash.
 * ========================================================= */
void
fs_compact()
{
  printf("\n[GC] Iniciando compactacion del sistema de archivos...\n");
  //Trabajamos sobre una copia del superbloque en RAM 
  MetaData_Table compact_meta;
  memcpy(&compact_meta, fs_meta, sizeof(MetaData_Table));
  
  /* --------------------------------------------------------
   *  Paso 1: Ordenamos las entradas OCCUPIED por offset
   *  usando burbuja sobre el arreglo RAM
   *  Las entradas no-OCCUPIED se empujan al final del arreglo
   *  para que el cursor de escritura solo itere sobre archivos validos
   * -------------------------------------------------------- */
  for (int i = 0; i < MAX_FILES - 1; i++) {
    for (int j = 0; j < MAX_FILES - 1 - i; j++) {
      int j_occ  = (compact_meta.entries[j].status   == STATUS_OCCUPIED);
      int jn_occ = (compact_meta.entries[j+1].status == STATUS_OCCUPIED);
      
      //Un archivo activo debe quedar antes de uno inactivo,
      //  entre dos activos, el de menor offset va primero
      int should_swap = 0;
      if (!j_occ && jn_occ)
	should_swap = 1;
      else if (j_occ && jn_occ && compact_meta.entries[j].offset > compact_meta.entries[j+1].offset)
	should_swap = 1;
      
      if (should_swap) {
	FileEntry tmp = compact_meta.entries[j];
	compact_meta.entries[j]   = compact_meta.entries[j+1];
	compact_meta.entries[j+1] = tmp;
      }
    }
  }
  
  /* --------------------------------------------------------
   *  Pasos 2 y 3: Reubicar cada archivo OCCUPIED de forma contigua
   *  write_cursor apunta al siguiente byte libre disponible
   *  en la Flash de datos (empieza en el sector 1 del FS)
   * -------------------------------------------------------- */
  uint32_t write_cursor = FS_BASE_OFFSET + FLASH_SECTOR_SIZE;
  
  for (int i = 0; i < MAX_FILES; i++) {
    if (compact_meta.entries[i].status != STATUS_OCCUPIED)
      continue;
    uint32_t old_offset = compact_meta.entries[i].offset;
    uint32_t file_size  = compact_meta.entries[i].size;
    uint32_t new_offset = write_cursor; //Posicion actual del cursor de escritura
    //Avanzamos el cursor al siguiente limite de sector libre
    uint32_t aligned = ((new_offset + file_size + FLASH_SECTOR_SIZE - 1)
			/ FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
    write_cursor = aligned;
    
    //Si el archivo ya esta en el lugar correcto, no lo movemos
    if (old_offset == new_offset) {
      printf("[GC] '%s' ya esta en 0x%08X, sin movimiento.\n",
	     compact_meta.entries[i].name, new_offset);
      continue;
    }
    
    //Buffer temporal alineado a pagina para la copia de datos
    uint32_t program_size = ((file_size + FLASH_PAGE_SIZE - 1)
			     / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;
    uint8_t tmp_buf[program_size];
    memset(tmp_buf, 0xFF, program_size);
    
    // Leemos datos desde la direccion XIP antigua (ventana de solo lectura)
    const uint8_t *src = (const uint8_t *)(XIP_BASE + old_offset);
    memcpy(tmp_buf, src, file_size); //Copiamos a RAM antes de borrar
    
    // Calculamos cuantos sectores borrar en el destino 
    uint32_t dst_erase = ((file_size + FLASH_SECTOR_SIZE - 1)
			  / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
    
    // Borramos el area de destino y escribir los datos copiados
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(new_offset, dst_erase); //Limpiamos destino
    flash_range_program(new_offset, tmp_buf, program_size); //Escribirmos
    restore_interrupts(ints);
    
    // Actualizamos el offset del archivo en la copia RAM
    compact_meta.entries[i].offset = new_offset;
    
    printf("[GC] '%s' movido 0x%08X -> 0x%08X (%u bytes).\n",
	   compact_meta.entries[i].name,
	   old_offset, new_offset, file_size);
  }
  
  /* --------------------------------------------------------
   *  Paso 4: Liberar todos los slots DELETED en el directorio.
   *  Se resetean a 0xFF (estado de Flash borrada = STATUS_FREE).
   * -------------------------------------------------------- */
  for (int i = 0; i < MAX_FILES; i++)
    if (compact_meta.entries[i].status == STATUS_DELETED)
      memset(&compact_meta.entries[i], 0xFF, sizeof(FileEntry));
  
  /* --------------------------------------------------------
   *  Paso 5: Persistir el superbloque compactado en la Flash
   * -------------------------------------------------------- */
  fs_flush_meta(&compact_meta);
  printf("[GC] Compactacion finalizada. "
	 "Espacio libre unificado desde 0x%08X.\n", write_cursor);
}


/*
  =========================================
  * DEMOSTRACION PRINCIPAL (no modificar) *
  =========================================
*/
int main () {
  stdio_init_all();
  sleep_ms(2000); // Esperar a que conecte el Monitor Serie USB
  printf("\n\n--- Iniciando Demostracion de PicoFS ---\n");

  // Demo Modulo 1
  printf("\n[Modulo 1]: Inicializacion\n");
  fs_init();

  printf("Formateando para iniciar la demo limpia...\n");
  fs_format();
  fs_init(); // Re-inicializar para mostrar que ahora si detecta el Magic Number

  // Demo Modulo 2
  printf("\n[Modulo 2]: Asignacion y Alineacion\n");
  fs_create("file1.txt");
  fs_create("file2.bin");
  fs_create("image.png");
  
  fs_dump();

  // Demo Modulo 3
  printf("\n[Modulo 3]: Read-Modify-Write\n");
  const char* str1 = "Hola Mundo PicoFS!";
  fs_write("file1.txt", (const uint8_t*)str1, strlen(str1) + 1);

  // Escritura grande para comprobar el calculo de sectores contiguos
  uint8_t dummy_bin[5000];
  memset(dummy_bin, 0x42, sizeof(dummy_bin));
  fs_write("file2.bin", dummy_bin, sizeof(dummy_bin)); // Requiere 2 sectores

  fs_dump();
  
  printf("\n[Test de lectura fs_read]:\n");
  char read_buf[64];
  fs_read("file1.txt" , (uint8_t*)read_buf, sizeof(read_buf));
  printf("Contenido leido de 'file1.txt': %s\n", read_buf);

  // Demo Modulo 4
  printf("\n[Modulo 4]: Borrado Logico\n");
  fs_delete("file1.txt");
  
  fs_dump();
  
  printf("\n--- Demostracion Finalizada ---\n");

  /* --------------------------------------------------------
   *  DEMO PUNTOS EXTRA
   * -------------------------------------------------------- */
  printf("\n\n=== DEMO PUNTOS EXTRA ===\n");
  
  /* Punto Extra 1: Prevencion de Desbordamiento
   *  Intentar escribir 12000 bytes en file2.bin, que esta entre
   *  file1.txt (DELETED) e image.png (OCCUPIED).
   *  image.png esta en el sector inmediatamente siguiente,
   *  por lo que la escritura debe ser bloqueada con error -2. */
  printf("\n[Extra 1]: Prevencion de Desbordamiento\n");
  uint8_t big_buf[12000]; /* 12 KB invade el sector de image.png */
  memset(big_buf, 0xBB, sizeof(big_buf));
  int ret = fs_write("file2.bin", big_buf, sizeof(big_buf));
  if (ret == -2)
    printf("[Extra 1] OK: escritura bloqueada correctamente.\n");
  
  /* Punto Extra 2: Garbage Collection
   *  Estado actual: file1.txt=DELETED, file2.bin=OCCUPIED, image.png=OCCUPIED.
   *  fs_compact debe cerrar el hueco de file1.txt
   *  desplazando file2.bin e image.png hacia adelante y liberar el slot. */
  printf("\n[Extra 2]: Compactacion / Garbage Collection\n");
  printf("Estado antes de compactar:\n");
  fs_dump();
  
  fs_compact();
  
  printf("Estado despues de compactar:\n");
  fs_dump();
  
  /* Verificar que los datos de file2.bin siguen integros
     despues de ser movidos fisicamente por el GC */
  printf("[Extra 2] Verificando integridad de file2.bin post-compactacion...\n");
  uint8_t verify_buf[5000];
  int bytes = fs_read("file2.bin", verify_buf, sizeof(verify_buf));
  int ok = 1;
  for (int i = 0; i < bytes; i++) {
    if (verify_buf[i] != 0xBB) { ok = 0; break; }
  }
  printf("[Extra 2] Integridad de file2.bin: %s\n", ok ? "OK" : "FALLO");
  
  printf("\n--- Demostracion Puntos Extra Finalizada ---\n");
  
  while (1) {
    tight_loop_contents();
  }
  
  return 0;
}
