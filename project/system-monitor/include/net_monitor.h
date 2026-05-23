#ifndef NET_MONITOR_H
#define NET_MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_IFACES 16 //maximas interfaces de red a monitorear
#define MAX_CONNS 512 //maximas conexiones TCP/UDP a escanear

/* IfaceStats:
   estadisticas de una interfaz de red /proc/net/dev */
typedef struct {
  char name[32];   //nombre de la interfaz: eth0, wlan0, lo...
  long rx_bytes;   //bytes recibidos desde que arranco el SO
  long tx_bytes;   //bytes transmitidos desde que arranco el SO
  long rx_packets; //paquetes recibidos
  long tx_packets; //paquetes transmitidos
  int is_virtual;  //1 si es loopback, bridge o virtual
} IfaceStats;

/* NetConn:
   una conexion TCP o UDP activa asociada a un proceso
   /proc/net/tcp + /proc/[pid]/fd/ para cruzar con PID */
typedef struct {
  char protocol[8];      //"TCP" o "UDP"
  char local_addr[48];   //direccion local ip:puerto en texto
  char remote_addr[48];  //direccion remota ip:puerto en texto
  char remote_host[128]; //hostname resuelto de la IP remota
  char state[16];        //ESTABLISHED, LISTEN, TIME_WAIT, etc
  int pid;               //PID del proceso dueno del socket
  char proc_name[256];   //nombre del proceso
  long inode;            //inodo del socket, sirve para cruzar datos
  /* campos extra, solo disponibles con root via nf_conntrack */
  long bytes_sent;      //bytes enviados en esta conexion
  long bytes_recv;      //bytes recibidos en esta conexion
  long packets_sent;
  long packets_recv;
  int has_traffic_data; //1 si pudimos leer nf_conntrack
} NetConn;

/* NetSnapshot:
   fotografia completa del estado de red en un instante */
typedef struct {
  IfaceStats ifaces[MAX_IFACES];
  int iface_count;
  NetConn conns[MAX_CONNS];
  int conn_count;
  //Totales globales sumados de todas las interfaces (excepto lo)
  long total_rx_bytes;
  long total_tx_bytes;
} NetSnapshot;

/* API publica */
int is_root();
void collect_net_ifaces(NetSnapshot *ns); //lee /proc/net/dev
void collect_net_conns(NetSnapshot *ns);  //lee /proc/net/tcp+udp
void collect_conntrack(NetSnapshot *ns);
void print_net_snapshot(const NetSnapshot *ns);

#endif /* NET_MONITOR_H */
