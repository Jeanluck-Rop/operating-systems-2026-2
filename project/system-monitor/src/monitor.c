#include "monitor.h"

/* Funcion de comparacion para qsort
   qsort necesita una funcion que reciba dos apuntadores void* y devuelva:
     -Negativo si a < b
     -0 si son iguales
     -Positivo si a > b
   Aqui queremos orden DESCENDENTE por memory_kb (mayor primero),
   por eso comparamos b contra a, no a contra b
   El truco (b > a) - (b < a) evita overflow que podria ocurrir
   con una resta directa b - a cuando los valores son muy grandes */
static int
cmp_mem(const void *a,
	const void *b)
{
  //Cast de void* al tipo real para poder acceder a los campos
  const ProcessInfo *process_a = (const ProcessInfo *)a;
  const ProcessInfo *process_b = (const ProcessInfo *)b;
  return (process_b->memory_kb > process_a->memory_kb) - (process_b->memory_kb < process_a->memory_kb);
}

/* Funcion que convierte el caracter de estado del kernel a texto legible
   El kernel usa un solo char para el estado; estos son los valores
   definidos en include/linux/sched.h del kernel de Linux */
static const char*
state_str(char c)
{
  switch (c) {
  case 'R':
    return "running";
  case 'S':
    return "sleeping";
  case 'D':
    return "waiting";
  case 'Z':
    return "zombie";
  case 'T':
    return "stopped";
  default:
    return "other";
  }
}



/* Funcion para leer /proc/meminfo y extrae las metricas de memoria del sistema /proc/meminfo
   es generado por el kernel en cada lectura; cada linea tiene formato "Campo: valor kB" */
void
collect_memory(SystemSnapshot *system_snapshot)
{
  FILE *f = fopen("/proc/meminfo", "r");  //abrimos el archivo virtual del kernel
  if (!f)
    return; //Si no se puede abrir, salir sin modificar el snapshot

  char line[128]; //buffer para leer una linea a la vez
  long value; //variable temporal donde sscanf deposita el numero

  // Leemos linea por linea hasta EOF
  while (fgets(line, sizeof(line), f)) {
    /* sscanf intenta hacer match del patron en la linea
       Devuelve el numero de campos que pudo leer (0 o 1)
       solo guardamos el valor si el match fue exitoso (== 1) */
    if (sscanf(line, "MemTotal: %ld kB", &value) == 1)
      system_snapshot->memory_total_kb = value;
    else if (sscanf(line, "MemFree: %ld kB", &value) == 1)
      system_snapshot->memory_free_kb = value;
    else if (sscanf(line, "Buffers: %ld kB", &value) == 1)
      system_snapshot->memory_buffers_kb = value;
    else if (sscanf(line, "Cached: %ld kB", &value) == 1)
      system_snapshot->memory_cached_kb = value;
    else if (sscanf(line, "SwapTotal: %ld kB", &value) == 1)
      system_snapshot->swap_total_kb = value;
    else if (sscanf(line, "SwapFree: %ld kB", &value) == 1)
      system_snapshot->swap_used_kb = system_snapshot->swap_total_kb - value;
  }
  
  fclose(f); //siempre cerrar el archivo para liberar el descriptor

  /* Calculamos memoria realmente usada buffers y cached son reutilizables
     por el kernel cuando hay presion de memoria, por eso no se cuentan
     como "en uso real" por aplicaciones */
  system_snapshot->memory_used_kb = system_snapshot->memory_total_kb
                                    - system_snapshot->memory_free_kb
                                    - system_snapshot->memory_buffers_kb
                                    - system_snapshot->memory_cached_kb;
}



/* Funcion para iterar /proc buscando directorios numericos (cada uno es un PID)
   y extrae informacion de cada proceso vivo */
void
collect_processes(SystemSnapshot *system_snapshot)
{
  DIR *d = opendir("/proc"); //abrimos el directorio /proc como un iterador
  if (!d)
    return;

  struct dirent *entry; //entrada actual del directorio
  system_snapshot->process_count = 0;

   /* readdir devuelve la siguiente entrada cada vez que se llama,
      NULL cuando no quedan mas entradas */
  while ((entry = readdir(d)) != NULL && system_snapshot->process_count < MAX_PROCS) {
    
    /* Solo nos interesan entradas cuyo nombre empieza con digito
      /proc tiene directorios como "net", "sys", "self" que ignoramos */
    if (!isdigit(entry->d_name[0]))
      continue;

    int pid = atoi(entry->d_name);
    ProcessInfo p;
    memset(&p, 0, sizeof(p)); //inicializamos todos los campos a cero
    p.process_id = pid;

    char path[64];  //buffer para construir rutas como /proc/1234/comm
    char line[256]; //buffer para leer lineas de los archivos de status
    FILE *f;

    /* Contiene el comando completo con argumentos separados por '\0'
       ej: "/usr/bin/firefox\0--no-sandbox\0" 
       fread en lugar de fgets porque puede haber '\0' en medio */
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    f = fopen(path, "r");
    if (f) {
      int len = fread(p.cmdline, 1, sizeof(p.cmdline) - 1, f);
      fclose(f);
      //Reemplazamos separadores '\0' por espacios para poder imprimirlo
      for (int i = 0; i < len - 1; i++)
	if (p.cmdline[i] == '\0') p.cmdline[i] = ' ';
      p.cmdline[len] = '\0';
    }

    /* Nombre corto del ejecutable, truncado a 15 chars por el kernel
       siempre disponible, incluso para hilos del kernel */
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    f = fopen(path, "r");
    if (!f)
      continue; //si no podemos leer comm, el proceso ya termino saltar
    fgets(p.name, sizeof(p.name), f);
    p.name[strcspn(p.name, "\n")] = '\0';  //quitamos el newline final
    fclose(f);

    /* Archivo con multiples campos en formato "Campo:\tvalor"
       leemos State, PPid, VmRSS y VmSize de aqui */
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    if (!f)
      continue;
    while (fgets(line, sizeof(line), f)) {
      long val;
      char c;
      int iv;
      if (sscanf(line, "State:\t%c", &c) == 1)
	p.state = c;
      else if (sscanf(line, "PPid:\t%d", &iv) == 1)
	p.parent_process_id = iv;
      else if (sscanf(line, "VmRSS:\t%ld kB", &val) == 1)
	p.memory_kb = val;
      else if (sscanf(line, "VmSize:\t%ld kB", &val) == 1)
	p.vm_size_kb = val;
    }
    fclose(f);

    /* Filtramos procesos del kernel: no tienen memoria de usuario (RSS = 0)
       hilos como kworker, migration, rcu_sched no aportan informacion util */
    if (p.memory_kb == 0)
      continue;

    //Leemos el comm del proceso padre usando su PPID
    if (p.parent_process_id > 0) {
      snprintf(path, sizeof(path), "/proc/%d/comm", p.parent_process_id);
      f = fopen(path, "r");
      if (f) {
	fgets(p.parent_name, sizeof(p.parent_name), f);
	p.parent_name[strcspn(p.parent_name, "\n")] = '\0';
	fclose(f);
      }
    }

    //Guardamos el proceso en el arreglo y avanzamos el contador
    system_snapshot->processes[system_snapshot->process_count] = p;
    system_snapshot->process_count++;
  }
  closedir(d); //Liberamos el descriptor del directorio
}



/* Funcion para imprimir el snapshot completo, primero metricas de memoria,
   luego tabla de procesos ordenada de mayor a menor RSS */
void
print_snapshot(const SystemSnapshot *system_snapshot)
{
  /* --- seccion de memoria --- */
  printf("\n============== Memoria ==============\n");
  printf("  Total:   %10ld KB  (%ld MB)\n",
	 system_snapshot->memory_total_kb, system_snapshot->memory_total_kb / 1024);
  printf("  Usada:   %10ld KB  (%ld MB)\n",
	 system_snapshot->memory_used_kb, system_snapshot->memory_used_kb / 1024);
  printf("  Libre:   %10ld KB  (%ld MB)\n",
	 system_snapshot->memory_free_kb, system_snapshot->memory_free_kb / 1024);
  printf("  Buffers: %10ld KB  (%ld MB)\n",
	 system_snapshot->memory_buffers_kb, system_snapshot->memory_buffers_kb / 1024);
  printf("  Cache:   %10ld KB  (%ld MB)\n",
	 system_snapshot->memory_cached_kb,  system_snapshot->memory_cached_kb / 1024);
  printf("  Swap:    %10ld KB usados / %ld KB total\n",
	 system_snapshot->swap_used_kb, system_snapshot->swap_total_kb);

  /* --- procesos ordenados por RSS --- */
  //Usamos memcpy para no modificar el arreglo original del snapshot;
  //qsort ordena en sitio usando cmp_mem como criterio
  ProcessInfo sorted[MAX_PROCS];
  memcpy(sorted, system_snapshot->processes, system_snapshot->process_count * sizeof(ProcessInfo));
  qsort(sorted, system_snapshot->process_count, sizeof(ProcessInfo), cmp_mem);

  printf("\n======================= Procesos con memoria (%d) =======================\n",
	 system_snapshot->process_count);
  printf("%-6s  %-6s  %-16s  %-10s  %-10s  %-8s  %-15s  %s\n",
	 "PID", "PPID", "Nombre", "RSS KB", "VmSize KB", "Estado", "Padre", "Comando");
  printf("%-6s  %-6s  %-16s  %-10s  %-10s  %-8s  %-15s  %s\n",
	 "──────", "──────", "────────────────", "──────────",
	 "──────────", "────────", "───────────────", "──────────────────────────────");

  for (int i = 0; i < system_snapshot->process_count; i++) {
    const ProcessInfo *p = &sorted[i];

    //Truncamos cmdline a 40 chars, si esta vacio usar name como fallback
    char cmd_trunc[41];
    snprintf(cmd_trunc, sizeof(cmd_trunc), "%.40s",
	     p->cmdline[0] ? p->cmdline : p->name);
    
    printf("%-6d  %-6d  %-16s  %-10ld  %-10ld  %-8s  %-15s  %s\n",
	   p->process_id,
	   p->parent_process_id,
	   p->name,
	   p->memory_kb,
	   p->vm_size_kb,
	   state_str(p->state),
	   p->parent_name,
	   cmd_trunc);
  }

  //Resumen final
  if (system_snapshot->process_count > 0) {
    printf("\n  Mayor consumidor: %s  (PID %d)  %ld KB\n",
	   sorted[0].name, sorted[0].process_id, sorted[0].memory_kb);
    printf("  Menor consumidor: %s  (PID %d)  %ld KB\n",
	   sorted[system_snapshot->process_count-1].name,
	   sorted[system_snapshot->process_count-1].process_id,
	   sorted[system_snapshot->process_count-1].memory_kb);
  }
}
