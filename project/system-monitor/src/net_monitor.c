#include "net_monitor.h"
#include <netdb.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <arpa/inet.h> //inet_ntop, htonl, para convertir IPs hex a texto
#include <sys/socket.h>
#include <netinet/in.h>


/* Funcion que verifica si es root
   getuid() == 0 significa proceso corriendo como root */
int
is_root(void)
{
  return getuid() == 0;
}


/* Funcion que convierte una IP en formato hexadecimal del kernel a texto legible
   (ej: "0101A8C0" a "192.168.1.1")
   /proc/net/tcp guarda IPs en hex little-endian  */
static void
hex_to_ip(const char *hex,
	  char *out,
	  size_t out_size)
{
  unsigned int addr;
  sscanf(hex, "%X", &addr); //leemos el hex como entero sin signo
  struct in_addr in;
  in.s_addr = addr;         //asignamos sin reordenar bytes
  //inet_ntoa convierte struct in_addr a "A.B.C.D"
  snprintf(out, out_size, "%s", inet_ntoa(in));
}

/* Funcin que convierte puerto hex del kernel a entero decimal */
static int
hex_to_port(const char *hex)
{
  int port;
  sscanf(hex, "%X", &port);
  return port;
}


/* Texto del estado TCP segun el numero que usa el kernel en /proc/net/tcp */
static const char*
tcp_state_str(int st)
{
  switch (st) {
  case 1:
    return "ESTABLISHED";
  case 2:
    return "SYN_SENT";
  case 3:
    return "SYN_RECV";
  case 4:
    return "FIN_WAIT1";
  case 5:
    return "FIN_WAIT2";
  case 6:
    return "TIME_WAIT";
  case 7:
    return "CLOSE";
  case 8:
    return "CLOSE_WAIT";
  case 9:
    return "LAST_ACK";
  case 10:
    return "LISTEN";
  case 11:
    return "CLOSING";
  default:
    return "UNKNOWN";
  }
}


/* Funcion que  resuelve una IP en texto a su hostname
   getnameinfo hace una consulta DNS inversa, si falla
   (sin internet, IP privada, timeout) devuelve la IP original
   NI_NOFQDN recorta el dominio completo al nombre corto */
static void
resolve_hostname(const char *ip_str,
		 char *out,
		 size_t out_size)
{
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  
  //Convertimos texto "1.2.3.4" a estructura de red
  if (inet_pton(AF_INET, ip_str, &sa.sin_addr) != 1) {
    strncpy(out, ip_str, out_size - 1);
    return;
  }
  
  char host[128];
  int result = getnameinfo(
			   (struct sockaddr *)&sa, sizeof(sa),
			   host, sizeof(host),
			   NULL, 0,          //no nos interesa el nombre del servicio/puerto 
			   NI_NOFQDN         //nombre corto
			   );
  if (result == 0 && strcmp(host, ip_str) != 0)
    strncpy(out, host, out_size - 1);
  else
    strncpy(out, ip_str, out_size - 1); //fallback a la IP numerica
  
  out[out_size - 1] = '\0';
}


/* Format_bytes:
   convierte bytes crudos a "1.2 KB", "3.4 MB", etc */
static void
format_bytes(long bytes,
	     char *out,
	     size_t out_size)
{
  if (bytes >= 1024 * 1024)
    snprintf(out, out_size, "%.1f MB", bytes / (1024.0 * 1024.0));
  else if (bytes >= 1024)
    snprintf(out, out_size, "%.1f KB", bytes / 1024.0);
  else
    snprintf(out, out_size, "%ld B", bytes);
}


/* Funcion que lee /proc/net/dev linea a linea
   formato del archivo (sin encabezados):
   "  eth0: rx_bytes rx_pkts rx_err ... tx_bytes tx_pkts tx_err ..."  */
void
collect_net_ifaces(NetSnapshot *ns)
{
  FILE *f = fopen("/proc/net/dev", "r");
  if (!f)
    return;
  
  char line[256];
  ns->iface_count = 0;
  ns->total_rx_bytes = 0;
  ns->total_tx_bytes = 0;
  
  //Saltar las dos primeras lineas de encabezado
  fgets(line, sizeof(line), f);
  fgets(line, sizeof(line), f);
  
  while (fgets(line, sizeof(line), f) && ns->iface_count < MAX_IFACES) {
    IfaceStats *iface = &ns->ifaces[ns->iface_count];
    
    int matched = sscanf(line,
			 " %31[^:]: %ld %ld %*ld %*ld %*ld %*ld %*ld %*ld %ld %ld",
			 iface->name,
			 &iface->rx_bytes, &iface->rx_packets,
			 &iface->tx_bytes, &iface->tx_packets);
    
    if (matched != 5)
      continue;
    
    //Marcamos interfaces virtuales
    iface->is_virtual = (
			 strcmp (iface->name, "lo")      == 0 ||
			 strncmp(iface->name, "br-",  3) == 0 ||
			 strncmp(iface->name, "veth", 4) == 0 ||
			 strncmp(iface->name, "virbr",5) == 0 ||
			 strcmp (iface->name, "docker0") == 0
			 );
    
    //Acumulamos totales solo de interfaces fisicas reales
    if (!iface->is_virtual) {
      ns->total_rx_bytes += iface->rx_bytes;
      ns->total_tx_bytes += iface->tx_bytes;
    }
    
    ns->iface_count++;
  }
  fclose(f);
}


/* Funcion que lee /proc/net/tcp y /proc/net/udp
   para obtener conexiones activas, luego cruza los inodos con
   /proc/[pid]/fd/ para encontrar el PID */
void
collect_net_conns(NetSnapshot *ns)
{
  ns->conn_count = 0;
  
  //Los dos archivos tienen el mismo formato; los procesamos en loop
  const char *files[] = { "/proc/net/tcp", "/proc/net/udp" };
  const char *protocols[] = { "TCP", "UDP" };
  
  for (int fi = 0; fi < 2 && ns->conn_count < MAX_CONNS; fi++) {
    FILE *f = fopen(files[fi], "r");
    if (!f)
      continue;
    
    char line[512];
    fgets(line, sizeof(line), f); //saltamos encabezado
    
    while (fgets(line, sizeof(line), f) && ns->conn_count < MAX_CONNS) {
      /* Formato de cada linea en /proc/net/tcp:
	 sl  local_address rem_address st tx_queue rx_queue
	 tr tm->when retrnsmt uid timeout inode */
      char local_hex[12], remote_hex[12];
      char local_port_hex[6], remote_port_hex[6];
      int state_num;
      long inode;
      int matched = sscanf(line,
			   " %*d: %8[^:]:%4s %8[^:]:%4s %X %*s %*s %*s %*d %*d %ld",
			   local_hex,  local_port_hex,
			   remote_hex, remote_port_hex,
			   &state_num, &inode);
      
      if (matched != 6)
	continue;
      
      NetConn *c = &ns->conns[ns->conn_count];
      strncpy(c->protocol, protocols[fi], sizeof(c->protocol) - 1);
      c->inode = inode;
      c->pid = -1; //aun sin PID se asigna abajo
      
      //Convertimos IPs y puertos de hex a texto legible
      char ip[INET_ADDRSTRLEN];
      hex_to_ip(local_hex, ip, sizeof(ip));
      snprintf(c->local_addr,  sizeof(c->local_addr),
	       "%s:%d", ip, hex_to_port(local_port_hex));
      
      hex_to_ip(remote_hex, ip, sizeof(ip));
      snprintf(c->remote_addr, sizeof(c->remote_addr),
	       "%s:%d", ip, hex_to_port(remote_port_hex));
      
      strncpy(c->state, tcp_state_str(state_num), sizeof(c->state) - 1);
      
      //Resolvemos hostname solo para ESTABLISHED con destino real
      if (state_num == 1 &&
	  strncmp(remote_hex, "00000000", 8) != 0) {
	char remote_ip[INET_ADDRSTRLEN];
	hex_to_ip(remote_hex, remote_ip, sizeof(remote_ip));
	resolve_hostname(remote_ip, c->remote_host, sizeof(c->remote_host));
      } else {
	strncpy(c->remote_host, c->remote_addr, sizeof(c->remote_host) - 1);
      }
      
      ns->conn_count++;
    }
    fclose(f);
  }
  
  /* Cruzamos inodos con PIDs
     cada proceso tiene sus file descriptors en /proc/[pid]/fd/
     los sockets aparecen como symlinks con destino "socket:[inode]"
     iteramos todos los PIDs y todos sus fd hasta encontrar coincidencia */
  DIR *proc_dir = opendir("/proc");
  if (!proc_dir)
    return;
  
  struct dirent *proc_entry;
  while ((proc_entry = readdir(proc_dir)) != NULL) {
    if (!isdigit(proc_entry->d_name[0]))
      continue;
    
    int pid = atoi(proc_entry->d_name);
    char fd_path[64];
    snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd", pid);
    
    DIR *fd_dir = opendir(fd_path);
    if (!fd_dir)
      continue;  //sin permisos para este proceso, saltamos

    struct dirent *fd_entry;
    while ((fd_entry = readdir(fd_dir)) != NULL) {
      if (fd_entry->d_name[0] == '.')
	continue;
      
      //Leemos el symlink: apunta a "socket:[12345]" si es un socket
      char link_path[128], link_target[128];
      snprintf(link_path, sizeof(link_path),
	       "/proc/%d/fd/%s", pid, fd_entry->d_name);

      ssize_t len = readlink(link_path, link_target, sizeof(link_target) - 1);
      if (len < 0)
	continue;
      link_target[len] = '\0';
      
      //Verificamos que el symlink apunta a un socket
      if (strncmp(link_target, "socket:[", 8) != 0)
	continue;
      
      //Extraemos el numero de inodo del string "socket:[12345]"
      long sock_inode;
      sscanf(link_target, "socket:[%ld]", &sock_inode);
      
      //Buscamos este inodo en nuestro arreglo de conexiones 
      for (int i = 0; i < ns->conn_count; i++) {
	if (ns->conns[i].inode == sock_inode && ns->conns[i].pid == -1) {
	  ns->conns[i].pid = pid;
	  //Leemos nombre del proceso
	  char comm_path[64];
	  snprintf(comm_path, sizeof(comm_path), "/proc/%d/comm", pid);
	  FILE *cf = fopen(comm_path, "r");
	  if (cf) {
	    fgets(ns->conns[i].proc_name, sizeof(ns->conns[i].proc_name), cf);
	    ns->conns[i].proc_name[strcspn(ns->conns[i].proc_name, "\n")] = '\0';
	    fclose(cf);
	  }
	}
      }
    }
    closedir(fd_dir);
  }
  closedir(proc_dir);
}


/* Funcion que enriquece las conexiones del snapshot con datos de trafico
   leyendo /proc/net/nf_conntrack, tabla interna del netfilter del kernel
   Solo accesible con privilegios root, requiere que nf_conntrack_acct=1
   este habilitado para obtener contadores de bytes y paquetes
   Si el modulo no tiene contabilidad activa, has_traffic_data queda en 0
   y no se imprime la linea de trafico en print_net_snapshot */
void
collect_conntrack(NetSnapshot *ns)
{
  /* Abrimos la tabla de seguimiento de conexiones del kernel
     este archivo solo existe si el modulo nf_conntrack esta cargado */
  FILE *f = fopen("/proc/net/nf_conntrack", "r");
  if (!f) {
    fprintf(stderr, "  [!] nf_conntrack no disponible\n");
    return;
  }

  /* buffer para una linea completa de nf_conntrack
     las lineas pueden ser largas, 1024 es suficiente */
  char line[1024];
  
  while (fgets(line, sizeof(line), f)) {
    /* nf_conntrack tambien registra conexiones IPv6
       solo procesamos IPv4 por ahora, las lineas IPv6 empiezan con "ipv6" */
    if (strncmp(line, "ipv4", 4) != 0)
      continue;
    
    //Campos que vamos a extraer de la linea 
    char src_orig[48] = {0}; //IP origen del trafico saliente
    char dst_orig[48] = {0}; //IP destino del trafico saliente
    int sport_orig = 0;      //puerto origen
    int dport_orig = 0;      //puerto destino
    
    /* Usamos strstr en lugar de sscanf directo sobre la linea completa
       porque el formato de nf_conntrack tiene campos opcionales y el
       orden puede variar segun version del kernel y tipo de protocolo
       strstr encuentra el campo por nombre sin importar su posicion */
    /* extraemos IP origen: buscamos "src=" y leemos la palabra siguiente
       +4 salta los 4 caracteres de "src=" para apuntar directo al valor.
       %47s lee hasta el siguiente espacio, exactamente una IP */
    char *p;
    p = strstr(line, "src=");
    if (!p)
      continue;
    sscanf(p + 4, "%47s", src_orig);
    
    //extraemos IP destino con el mismo patron
    p = strstr(line, "dst=");
    if (!p)
      continue;
    sscanf(p + 4, "%47s", dst_orig);
    
    //extraer puerto origen: +6 salta "sport="
    p = strstr(line, "sport=");
    if (!p)
      continue;
    sscanf(p + 6, "%d", &sport_orig);
    
    //extraer puerto destino: +6 salta "dport="
    p = strstr(line, "dport=");
    if (!p)
      continue;
    sscanf(p + 6, "%d", &dport_orig);
    
    /* intentar leer contadores de bytes y paquetes.
       estos campos solo existen si nf_conntrack_acct esta activo.
       cada conexion tiene DOS entradas de bytes= y packets=:
       primera = trafico original (lo que nosotros enviamos)
       segunda = trafico de respuesta (lo que recibimos) */
    long bytes_orig = 0, bytes_reply = 0;
    long pkts_orig = 0, pkts_reply = 0;
    int has_acct = 0; //bandera: 1 si encontramos bytes=
    
    /* recorremos la linea buscando todas las ocurrencias de "bytes="
       occ lleva la cuenta para distinguir primera de segunda aparicion */
    char *cursor = line;
    int occ = 0;
    while ((cursor = strstr(cursor, "bytes=")) != NULL) {
      long val;
      sscanf(cursor + 6, "%ld", &val); //+6 salta "bytes="
      if (occ == 0) {
	bytes_orig = val;
	has_acct = 1; //confirmamos que hay contabilidad
      } else {
	bytes_reply = val;
      }
      occ++;
      cursor++; //avanzar 1 para no quedarse en la misma posicion
    }
    
    //mismo proceso para paquetes
    cursor = line; occ = 0;
    while ((cursor = strstr(cursor, "packets=")) != NULL) {
      long val;
      sscanf(cursor + 8, "%ld", &val); //+8 salta "packets="
      if (occ == 0)
	pkts_orig  = val;
      else
	pkts_reply = val;
      occ++;
      cursor++;
    }
    
    /* Construimos los strings "IP:puerto" igual que como los guarda
       collect_net_conns en local_addr y remote_addr
       esto nos permite hacer el match por comparacion de strings */
    char local_str[48], remote_str[48];
    snprintf(local_str,  sizeof(local_str), "%s:%d", src_orig, sport_orig);
    snprintf(remote_str, sizeof(remote_str), "%s:%d", dst_orig, dport_orig);
    
    /* Buscamos en el arreglo de conexiones la que coincida con esta
       entrada de conntrack, cuando la encontramos, copiamos los
       contadores y marcamos has_traffic_data para que print_net_snapshot
       sepa que tiene datos validos que mostrar */
    for (int i = 0; i < ns->conn_count; i++) {
      NetConn *c = &ns->conns[i];
      if (strcmp(c->local_addr,  local_str)  == 0 &&
	  strcmp(c->remote_addr, remote_str) == 0) {
	c->bytes_sent = bytes_orig;
	c->bytes_recv = bytes_reply;
	c->packets_sent = pkts_orig;
	    c->packets_recv = pkts_reply;
	    c->has_traffic_data = has_acct; //0 si no hay acct
	    break; //cada conexion es unica, no seguir buscando
      }
    }
  }
  
  fclose(f);
}


/* Comparadores para qsort */
static int
cmp_rx(const void *a,
       const void *b)
{
  const IfaceStats *ia = (const IfaceStats *)a;
  const IfaceStats *ib = (const IfaceStats *)b;
  return (ib->rx_bytes > ia->rx_bytes) - (ib->rx_bytes < ia->rx_bytes);
}

/* Funcion que imprime interfaces, conexiones activas y resumen global */
void print_net_snapshot(const NetSnapshot *ns) {

  /* --- Interfaces --- */
  printf("\n============== Interfaces de red ==============\n");
  printf("%-15s  %15s  %12s  %15s  %12s\n",
	 "Interfaz", "RX bytes", "RX paquetes", "TX bytes", "TX paquetes");
  printf("%-15s  %15s  %12s  %15s  %12s\n",
	 "───────────────", "───────────────", "────────────",
	 "───────────────", "────────────");
  
  for (int i = 0; i < ns->iface_count; i++) {
    const IfaceStats *iface = &ns->ifaces[i];
    printf("%-15s  %15ld  %12ld  %15ld  %12ld\n",
	   iface->name,
	   iface->rx_bytes, iface->rx_packets,
	   iface->tx_bytes, iface->tx_packets);
  }
  
  /* --- interfaces virtuales (colapsadas en una linea) --- */
  printf("\n=== Interfaces virtuales (lo, docker, bridge) ===\n");
  for (int i = 0; i < ns->iface_count; i++) {
    const IfaceStats *iface = &ns->ifaces[i];
    if (!iface->is_virtual)
      continue;
    printf("  %-12s  RX: %ld bytes  TX: %ld bytes\n",
	   iface->name, iface->rx_bytes, iface->tx_bytes);
  }
  
  /* Mayor y menor por RX entre interfaces no-loopback */
  IfaceStats sorted_ifaces[MAX_IFACES];
  int real_count = 0;
  for (int i = 0; i < ns->iface_count; i++)
    if (strcmp(ns->ifaces[i].name, "lo") != 0)
      sorted_ifaces[real_count++] = ns->ifaces[i];
  
  if (real_count > 0) {
    qsort(sorted_ifaces, real_count, sizeof(IfaceStats), cmp_rx);
    printf("\n  Mayor RX: %-10s  %ld bytes\n",
	   sorted_ifaces[0].name, sorted_ifaces[0].rx_bytes);
    printf("  Menor RX: %-10s  %ld bytes\n",
	   sorted_ifaces[real_count-1].name,
	   sorted_ifaces[real_count-1].rx_bytes);
  }
  
  printf("\n  Total RX: %ld bytes  (%ld KB)\n",
	 ns->total_rx_bytes, ns->total_rx_bytes / 1024);
  printf("  Total TX: %ld bytes  (%ld KB)\n",
	 ns->total_tx_bytes, ns->total_tx_bytes / 1024);
  
  /* --- Conexiones Activas --- */
  printf("\n============== Conexiones activas (%d) ==============\n",
	 ns->conn_count);
  printf("%-5s  %-8s  %-25s  %-25s  %-14s  %s\n",
	 "PID", "Proto", "Local", "Remoto", "Estado", "Proceso");
  printf("%-5s  %-8s  %-25s  %-25s  %-14s  %s\n",
	 "─────", "────────", "─────────────────────────",
	 "─────────────────────────", "──────────────", "──────────────────");
  
  for (int i = 0; i < ns->conn_count; i++) {
    const NetConn *c = &ns->conns[i];
    /* Mostramos solo conexiones que pudimos asociar a un proceso
       y que no son solo LISTEN en 0.0.0.0 sin actividad real   */
    if (c->pid == -1)
      continue;

    //Filtrar UDP sin destino real 
    if (strcmp(c->protocol, "UDP") == 0 &&
	strncmp(c->remote_addr, "0.0.0.0", 7) == 0) continue;
    
    //Mostramos hostname si fue resuelto, si no la IP
    const char *remote_display =
      (c->remote_host[0] && strcmp(c->remote_host, c->remote_addr) != 0)
      ? c->remote_host
      : c->remote_addr;
    
    printf("%-5d  %-8s  %-25s  %-25s  %-14s  %s\n",
	   c->pid,
	   c->protocol,
	   c->local_addr,
	   c->remote_addr,
	   c->state,
	   c->proc_name);

    //Datos de trafico solo si tenemos nf_conntrack (root)
    if (c->has_traffic_data) {
      char sent[16], recv[16];
      format_bytes(c->bytes_sent, sent, sizeof(sent));
      format_bytes(c->bytes_recv, recv, sizeof(recv));
      printf("  ↑%-10s ↓%-10s  pkts ↑%ld/↓%ld",
	     sent, recv, c->packets_sent, c->packets_recv);
    }
    printf("\n");
  }
}
