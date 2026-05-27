#include "ui.h"
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>

/* Pares de color: definimos un numero por par
   para no usar "magic numbers" en el codigo */
#define COL_HEADER 1 //cyan, barra superior e inferior
#define COL_TITLE  2 //cyan, encabezados
#define COL_LOW    3 //verde, uso bajo o conexion ok
#define COL_MED    4 //amarillo, uso medio
#define COL_HIGH   5 //rojo, uso alto o proceso pesado
#define COL_NORMAL 6 //blanco, texto de filas normales

/* Ventanas ncurses:
   dividimos la pantalla en tres regiones fijas
   WINDOW* es el tipo de ncurses para una subventana con coordenadas propias */
static WINDOW *win_header;  //fila 0: titulo + modo + hora
static WINDOW *win_content; //filas 1..(rows-3): tabla scrolleable
static WINDOW *win_footer;  //ultimas 2 filas: resumen + atajos

/* Por si algun dia volvemos a implementar docker */
static int
in_docker(void)
{
  return access("/.dockerenv", F_OK) == 0;
}

/* comparador para qsort, debe estar en scope de modulo, no anidado,
   las funciones anidadas no son C99 estandar                         */
static int
cmp_mem_ui(const void *a,
	   const void *b)
{
  const ProcessInfo *pa = (const ProcessInfo *)a;
  const ProcessInfo *pb = (const ProcessInfo *)b;
  return (pb->memory_kb > pa->memory_kb) - (pb->memory_kb < pa->memory_kb);
}


/* Registramos todos los pares de color una sola vez al arrancar
   start_color() habilita el subsistema de colores de ncurses
   use_default_colors() permite -1 como "fondo transparente del terminal" */
static void
init_colors()
{
  start_color();
  use_default_colors();
  init_pair(COL_HEADER, COLOR_BLACK, COLOR_CYAN);
  init_pair(COL_TITLE, COLOR_CYAN, -1);
  init_pair(COL_LOW, COLOR_GREEN, -1);
  init_pair(COL_MED, COLOR_YELLOW, -1);
  init_pair(COL_HIGH, COLOR_RED, -1);
  init_pair(COL_NORMAL, COLOR_WHITE, -1);
}

/* Funcion que crea o recrea las tres ventanas ajustadas al tamano actual,
   se llama al inicio y cada vez que el usuario redimensiona la terminal
   newwin(filas, cols, fila_inicio, col_inicio) */
static void
create_windows()
{
  int rows, cols;
  getmaxyx(stdscr, rows, cols);

  //Destruimos ventanas previas si existen, caso redimension
  if (win_header) {
    delwin(win_header);
    win_header = NULL;
  }
  if (win_content) {
    delwin(win_content);
    win_content = NULL;
  }
  if (win_footer) {
    delwin(win_footer);
    win_footer = NULL;
  }

  //header: 1 fila de alto, ancho total, empieza en fila 0
  win_header = newwin(1, cols, 0, 0);
  //content: todo entre header y footer
  win_content = newwin(rows - 3, cols, 1, 0);
  //footer: 2 filas al final de la pantalla
  win_footer = newwin(2, cols, rows - 2, 0);
}

/* Dibujamos una barra de progreso de 'width' chars dentro de corchetes,
   usa ACS_BLOCK para el area llena y '.' para el area vacia,
   el color de los bloques cambia segun el porcentaje */
static void
draw_bar(WINDOW *win,
	 int pct,
	 int width)
{
  int filled = (int)((double)pct / 100.0 * width);

  waddch(win, '[');
  for (int i = 0; i < width; i++) {
    if (i < filled) {
      int col = pct > 85 ? COL_HIGH : pct > 60 ? COL_MED : COL_LOW;
      wattron(win, COLOR_PAIR(col));
      waddch(win, ACS_BLOCK);
      wattroff(win, COLOR_PAIR(col));
      wattron(win, COLOR_PAIR(COL_HEADER));
    } else {
      waddch(win, '.');
    }
  }
  waddch(win, ']');
}

/* HEADER estatico: fondo cyan, titulo a la izquierda, hora a la derecha */
static void
draw_header(DisplayMode mode)
{
  int cols = getmaxx(win_header);
  
  /* wbkgd pinta toda la ventana con el par de color indicado,
     sin esto las celdas vacias del header quedarian sin fondo */
  wbkgd(win_header, COLOR_PAIR(COL_HEADER));
  werase(win_header);
  
  const char *title = (mode == MODE_MEMORY)
    ? "System Monitor [Memory]"
    : "System Monitor [Net]";

  wattron(win_header, COLOR_PAIR(COL_HEADER) | A_BOLD);
  mvwprintw(win_header, 0, 1, "%s", title);
  
  //En draw_header(), despues del titulo
  if (getuid() == 0)
    mvwprintw(win_header, 0, (int)strlen(title) + 3, "[root]");
  
  //Hora actual alineada a la derecha
  time_t now = time(NULL);
  char timebuf[32];
  strftime(timebuf, sizeof(timebuf), "%a %d %b  %H:%M:%S", localtime(&now));
  mvwprintw(win_header, 0, cols - (int)strlen(timebuf) - 1, "%s", timebuf);
  wattroff(win_header, COLOR_PAIR(COL_HEADER) | A_BOLD);

  /* wnoutrefresh marca la ventana como pendiente de actualizar,
     NO escribe en pantalla todavia, eso lo hace doupdate() al final,
     esto evita parpadeo, todas las ventanas se vuelcan en un solo paso */
  wnoutrefresh(win_header);
}

/* Contenido MODO MEMORIA dibuja encabezado de:
   columnas + filas de procesos scrolleables */
static void
draw_content_memory(const SystemSnapshot *snap,
		    UIState *state)
{
  int rows, cols;
  getmaxyx(win_content, rows, cols);
  (void)cols;
  
  werase(win_content);
  
  //Encabezado de columnas fila 0 del content, nunca scrollea
  wattron(win_content, COLOR_PAIR(COL_TITLE) | A_BOLD);
  mvwprintw(win_content, 0, 0,
	    "%-6s  %-6s  %-16s  %-10s  %-10s  %-8s  %-15s  %s",
	    "PID", "PPID", "Nombre", "RSS KB",
	    "VmSize KB", "Estado", "Padre", "Comando");
  wattroff(win_content, COLOR_PAIR(COL_TITLE) | A_BOLD);

  //Separador horizontal bajo el encabezado
  mvwhline(win_content, 1, 0, ACS_HLINE, cols);

  //Ordenamo copia por RSS descendente, igual que en print_snapshot
  ProcessInfo sorted[MAX_PROCS];
  memcpy(sorted, snap->processes,
	 snap->process_count * sizeof(ProcessInfo));
  qsort(sorted, snap->process_count, sizeof(ProcessInfo), cmp_mem_ui);

  //Filas disponibles = total content - encabezado (fila 0) - separador (fila 1)
  int available = rows - 2;
  int total = snap->process_count;

  //Limitar scroll_offset para no pasarse del final de la lista
  if (state->scroll_offset > total - available)
    state->scroll_offset = total - available;
  if (state->scroll_offset < 0)
    state->scroll_offset = 0;

  for (int i = 0; i < available && (state->scroll_offset + i) < total; i++) {
    int screen_row = 2 + i;
    const ProcessInfo *p = &sorted[state->scroll_offset + i];

    //Color de la fila segun cuanta RAM usa el proceso
    int col = (p->memory_kb > 500000) ? COL_HIGH :
      (p->memory_kb > 100000) ? COL_MED : COL_NORMAL;

    //Estado en texto, mismo mapeo que state_str en monitor.c
    const char *st = p->state == 'R' ? "running"  :
      p->state == 'S' ? "sleeping" :
      p->state == 'D' ? "waiting"  :
      p->state == 'Z' ? "zombie"   : "other";

    //cmdline truncado a 35 chars, fallback al name si esta vacio
    char cmd[36];
    snprintf(cmd, sizeof(cmd), "%.35s", p->cmdline[0] ? p->cmdline : p->name);

    wattron(win_content, COLOR_PAIR(col));
    mvwprintw(win_content, screen_row, 0,
	      "%-6d  %-6d  %-16s  %-10ld  %-10ld  %-8s  %-15s  %s",
	      p->process_id, p->parent_process_id,
	      p->name, p->memory_kb, p->vm_size_kb,
	      st, p->parent_name, cmd);
    wattroff(win_content, COLOR_PAIR(col));
  }

  //Indicador de posicion de scroll en el borde derecho
  if (total > available) {
    int pct = (state->scroll_offset * 100)
      / (total - available > 0 ? total - available : 1);
    mvwprintw(win_content, rows / 2, cols - 5, " %3d%%", pct);
  }

  wnoutrefresh(win_content);
}

/* Contenido MODO RED:
   seccion fija de interfaces + seccion scrolleable de conexiones */
static void
draw_content_net(const NetSnapshot *ns,
		 UIState *state)
{
  int rows, cols;
  getmaxyx(win_content, rows, cols);
  (void)cols;

  werase(win_content);

  //Interfaces fisicas
  wattron(win_content, COLOR_PAIR(COL_TITLE) | A_BOLD);
  mvwprintw(win_content, 0, 0, "Interfaces fisicas:");
  wattroff(win_content, COLOR_PAIR(COL_TITLE) | A_BOLD);

  mvwprintw(win_content, 1, 0,
	    "%-12s  %15s  %15s  %12s  %12s",
	    "Interfaz", "RX bytes", "TX bytes",
	    "RX pkts", "TX pkts");
  mvwhline(win_content, 2, 0, ACS_HLINE, cols);

  int iface_row = 3;
  for (int i = 0; i < ns->iface_count; i++) {
    const IfaceStats *iface = &ns->ifaces[i];
    if (iface->is_virtual)
      continue;
    wattron(win_content, COLOR_PAIR(COL_LOW));
    mvwprintw(win_content, iface_row++, 0,
	      "%-12s  %15ld  %15ld  %12ld  %12ld",
	      iface->name,
	      iface->rx_bytes, iface->tx_bytes,
	      iface->rx_packets, iface->tx_packets);
    wattroff(win_content, COLOR_PAIR(COL_LOW));
  }

  //Separador antes de la seccion de conexiones
  int conn_start = iface_row + 1;
  mvwhline(win_content, conn_start - 1, 0, ACS_HLINE, cols);

  wattron(win_content, COLOR_PAIR(COL_TITLE) | A_BOLD);
  mvwprintw(win_content, conn_start, 0, "Conexiones activas:");
  wattroff(win_content, COLOR_PAIR(COL_TITLE) | A_BOLD);

  mvwprintw(win_content, conn_start + 1, 0,
	    "%-5s  %-5s  %-22s  %-28s  %-13s  %s",
	    "PID", "Proto", "Local", "Remoto/Host",
	    "Estado", "Proceso");
  mvwhline(win_content, conn_start + 2, 0, ACS_HLINE, cols);

  //Conexiones scrolleables
  int header_rows = conn_start + 3; //filas usadas por interfaces + enc
  int available = rows - header_rows;

  //Construimos lista de indices de conexiones visibles
  int visible[MAX_CONNS];
  int visible_count = 0;
  for (int i = 0; i < ns->conn_count; i++) {
    const NetConn *c = &ns->conns[i];
    if (c->pid == -1)
      continue;
    if (strcmp(c->protocol, "UDP") == 0 && strncmp(c->remote_addr, "0.0.0.0", 7) == 0)
      continue;
    visible[visible_count++] = i;
  }

  if (state->scroll_offset > visible_count - available)
    state->scroll_offset = visible_count - available;
  if (state->scroll_offset < 0)
    state->scroll_offset = 0;

  for (int i = 0; i < available && (state->scroll_offset + i) < visible_count; i++) {
    int ci = visible[state->scroll_offset + i];
    const NetConn *c = &ns->conns[ci];
    int screen_row = header_rows + i;

    //Color segun estado de la conexion
    int col = (strcmp(c->state, "ESTABLISHED") == 0) ? COL_LOW :
      (strcmp(c->state, "SYN_SENT") == 0) ? COL_MED : COL_NORMAL;

    //Mostramos hostname si fue resuelto y es distinto a la IP
    const char *remote = (c->remote_host[0] &&
			  strcmp(c->remote_host, c->remote_addr) != 0)
      ? c->remote_host : c->remote_addr;

    wattron(win_content, COLOR_PAIR(col));
    
    mvwprintw(win_content, screen_row, 0,
	      "%-5d  %-5s  %-22s  %-28s  %-13s  %-16s",
	      c->pid, c->protocol,
	      c->local_addr, remote,
	      c->state, c->proc_name);

    //Datos de trafico, solo si root y nf_conntrack_acct activo
    if (c->has_traffic_data) {
      //Format_bytes inline para no depender de funcion externa
      char sent[16], recv[16];
      if(c->bytes_sent >= 1024*1024)
	snprintf(sent, sizeof(sent), "%.1fMB", c->bytes_sent / (1024.0*1024.0));
      else if (c->bytes_sent >= 1024)
	snprintf(sent, sizeof(sent), "%.1fKB", c->bytes_sent / 1024.0);
      else
	snprintf(sent, sizeof(sent), "%ldB", c->bytes_sent);

      if (c->bytes_recv >= 1024*1024)
	snprintf(recv, sizeof(recv), "%.1fMB", c->bytes_recv / (1024.0*1024.0));
      else if (c->bytes_recv >= 1024)
	snprintf(recv, sizeof(recv), "%.1fKB", c->bytes_recv / 1024.0);
      else
	snprintf(recv, sizeof(recv), "%ldB", c->bytes_recv);

      wprintw(win_content, "  ^%-8s v%-8s p^%ld/v%ld",
	      sent, recv, c->packets_sent, c->packets_recv);
    }

    wattroff(win_content, COLOR_PAIR(col));
  }

  wnoutrefresh(win_content);
}

/* FOOTER MEMORIA, estatico: barra RAM + swap + atajos */
static void
draw_footer_memory(const SystemSnapshot *snap)
{
  wbkgd(win_footer, COLOR_PAIR(COL_HEADER));
  werase(win_footer);
  wattron(win_footer, COLOR_PAIR(COL_HEADER));

  //Barra de RAM
  long total = snap->memory_total_kb;
  long used = snap->memory_used_kb;
  int pct = total > 0 ? (int)((double)used / total * 100.0) : 0;

  mvwprintw(win_footer, 0, 1, "RAM %ld/%ld MB ", used / 1024, total / 1024);
  draw_bar(win_footer, pct, 20);
  wprintw(win_footer, " %d%%  |  "
	  "Swap %ld/%ld MB  |  procs: %d",
	  pct,
	  snap->swap_used_kb  / 1024,
	  snap->swap_total_kb / 1024,
	  snap->process_count);
  //Segunda fila, atajos de teclado
  mvwprintw(win_footer, 1, 1, "Arriba/Abajo: scroll   N: red   q: salir");

  wattroff(win_footer, COLOR_PAIR(COL_HEADER));
  wnoutrefresh(win_footer);
}


/* FOOTER RED, estatico: totales RX/TX + atajos */
static void
draw_footer_net(const NetSnapshot *ns)
{
  wbkgd(win_footer, COLOR_PAIR(COL_HEADER));
  werase(win_footer);
  wattron(win_footer, COLOR_PAIR(COL_HEADER));

  mvwprintw(win_footer, 0, 1,
	    "Total RX: %.1f MB   TX: %.1f MB   "
	    "Interfaces: %d   Conexiones: %d",
	    ns->total_rx_bytes / (1024.0 * 1024.0),
	    ns->total_tx_bytes / (1024.0 * 1024.0),
	    ns->iface_count,
	    ns->conn_count);
  mvwprintw(win_footer, 1, 1, "Arriba/Abajo: scroll   N: memoria   q: salir");

  wattroff(win_footer, COLOR_PAIR(COL_HEADER));
  wnoutrefresh(win_footer);
}

/* Loop principal */
void
ui_run()
{
  /* setlocale es obligatorio antes de initscr para caracteres Unicode
     como ACS_BLOCK, ACS_HLINE y los separadores de tabla */
  setlocale(LC_ALL, "");
  initscr();             //Tomamos control completo de la terminal
  cbreak();              //Input inmediato sin esperar Enter
  noecho();              //No mostramos teclas presionadas en pantalla
  keypad(stdscr, TRUE);  //Habilitamos KEY_UP, KEY_DOWN, KEY_RESIZE
  nodelay(stdscr, TRUE); //getch() no bloquea, devuelve ERR si no hay tecla
  curs_set(0);           //Ocultamos cursor parpadeante

  if (!has_colors()) {
    endwin();
    fprintf(stderr, "El terminal no soporta colores\n");
    return;
  }

  init_colors();
  create_windows();

  UIState state = { MODE_MEMORY, 0, 1 };

  //Recolectamos datos iniciales antes del primer frame
  SystemSnapshot mem_snap;
  NetSnapshot net_snap;
  memset(&mem_snap, 0, sizeof(mem_snap));
  memset(&net_snap, 0, sizeof(net_snap));

  collect_memory(&mem_snap);
  collect_processes(&mem_snap);
  collect_net_ifaces(&net_snap);
  collect_net_conns(&net_snap);
  if (is_root())
    collect_conntrack(&net_snap);

  //TICK: recolectamos datos cada 10 frames x 100ms = 1 segundo
  int tick_counter = 0;
  const int TICK = 10;

  while (state.running) {
    //Input de teclado sin bloqueo
    int ch = getch();
    switch (ch) {
    case 'q':
    case 'Q':
      state.running = 0;
      break;

    case 'n':
    case 'N':
      if (state.mode == MODE_MEMORY) {
        //Intentar cambiar a modo red
        if (getuid() != 0) {
	  //No somos root, necesitamos escalar
	  if (in_docker()) {
	    /* en Docker sin --privileged no hay forma de escalar.
	       mostrar mensaje claro al usuario */
	    def_prog_mode();
	    endwin();
	    printf("\n[sysmon] En Docker necesitas --privileged:\n\n");
	    printf("  docker run --rm -it \\\n");
	    printf("    --pid=host --network=host \\\n");
	    printf("    --privileged sysmon\n\n");
	    printf("Presiona Enter para continuar en modo memoria...\n");
	    fflush(stdout);
	    getchar();
	    reset_prog_mode();
	    refresh();
	    create_windows();
	    break;
	  }
	  
	  def_prog_mode();
	  endwin();
	  printf("\n[sysmon] El modo red requiere root.\n");
	  printf("[sysmon] Se solicitaran credenciales de sudo.\n\n");
	  fflush(stdout);
	  int ok = system("sudo -v 2>/dev/null");
	  reset_prog_mode();
	  refresh();
	  create_windows(); //Recreamos ventanas tras el reset

	  /* Autenticacion fallo, mostramos mensaje en footer,
	     guardamos un flag de error para mostrarlo */
	  if (ok != 0)
	    break; //No cambiamos de modo

	  /* Autenticacion ok, relanzar el proceso entero como sudo
	     para que getuid() == 0 y /proc/net/nf_conntrack sea accesible */
	  endwin();
	  printf("\n[sysmon] Autenticado. Relanzando con privilegios...\n");
	  fflush(stdout);

	  //Obtener el path del propio ejecutable
	  char self[256];
	  ssize_t len = readlink("/proc/self/exe", self, sizeof(self) - 1);
	  if (len > 0) {
	    self[len] = '\0';
	    char *args[] = { "sudo", self, NULL };
	    execvp("sudo", args);
	  }
	  //si execvp falla, salir
	  exit(EXIT_FAILURE);
        }

        //Ya somos root, cambiamos modo directamente
        state.mode = MODE_NET;
        state.scroll_offset = 0;
      } else {
        //Volvemos a modo memoria, no requiere root
        state.mode = MODE_MEMORY;
        state.scroll_offset = 0;
      }
      break;

    case KEY_UP:
      if (state.scroll_offset > 0)
	state.scroll_offset--;
      break;

    case KEY_DOWN:
      state.scroll_offset++;
      break;

    case KEY_RESIZE:
      //el usuario redimensiono la terminal, reconstruir ventanas
      create_windows();
      break;

    default:
      break;
    }

    //Dibujamos las tres regiones
    draw_header(state.mode);

    if (state.mode == MODE_MEMORY) {
      draw_content_memory(&mem_snap, &state);
      draw_footer_memory(&mem_snap);
    } else {
      draw_content_net(&net_snap, &state);
      draw_footer_net(&net_snap);
    }

    /* doupdate() vuelca todos los wnoutrefresh pendientes en un
       solo paso, de este modo evitamos parpadeo visible entre ventanas */
    doupdate();

    //Recolectamos datos frescos cada TICK frames */
    tick_counter++;
    if (tick_counter >= TICK) {
      tick_counter = 0;
      if (state.mode == MODE_MEMORY) {
	memset(&mem_snap, 0, sizeof(mem_snap));
	collect_memory(&mem_snap);
	collect_processes(&mem_snap);
      } else {
	memset(&net_snap, 0, sizeof(net_snap));
	collect_net_ifaces(&net_snap);
	collect_net_conns(&net_snap);
	if (is_root())
	  collect_conntrack(&net_snap);
      }
    }

    usleep(100 * 1000); //100 ms entre frames = 10 fps
  }

  //Restauramos terminal antes de salir, siempre obligatorio
  delwin(win_header);
  delwin(win_content);
  delwin(win_footer);
  endwin();
}
