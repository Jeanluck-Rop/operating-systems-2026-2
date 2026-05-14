#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

/* ====================================================================
   TODO 1:
   No incluimos bibliotecas adicionales dado que Polling no requiere
   ni hardware/irq.h (Interrupciones) ni hardware/dma.h (DMA)
   Ambos mecanismos los descartamos por el analisis del caso de estudio
   ==================================================================== */

//Definicion de parametros del hardware
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1
//Requerimiento del caso de estudio: Exactamente 16 bytes
#define KEY_LENGTH 16
//Buffer global para guardar la clave
char challenge_key[KEY_LENGTH + 1];
//Variable de control (Util si tu implementacion es asincrona)
volatile bool key_received = false ;

/* ================================================================
   TODO 2:
   No se define ninguna ISR pues con Polling y las interrupciones 
   del UART deshabilitadas, el hardware nunca generara una senial 
   de interrupcion, por lo que un handler aqui nunca seria invocado
   ================================================================ */


void
secure_boot_init()
{
  //Inicializa la salida estandar (para ver los printf en consola)
  stdio_init_all();
  
  /* ===================================================================
     TODO 3:
     Inicializacion Basica del UART
     Configuramos el periferico con el ID y el Baud Rate definidos
     para despues mapear los pines fisicos a su funcion de hardware UART
     =================================================================== */
  //Arrancamos el controlador UART0 del RP2040 con 115200 bps
  uart_init(UART_ID, BAUD_RATE);
  
  //Asignamos el pin 0 (TX) y el pin 1 (RX) a la funcion UART
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
  
  /* ==================================================================
     TODO 4:
     Configuracion especifica del mecanismo (Polling)
     Definimos dos acciones criticas derivadas del analisis del correo:
     =============================================================== */
  
  /* Deshabilitamos en el NVIC la IRQ del UART0
     Con esto implementamos la orden del auditor de seguridad "bloquear
     a nivel fisico cualquier evento o senal asincrona externa" */
  irq_set_enabled(UART0_IRQ, false); //El hardware no puede interrumpir al CPU

  /* Desactivamos el buffer FIFO del UART.
     Sin FIFO, cada byte queda disponible de inmediato en el
     registro de datos, lo que hace al sondeo completamente
     determinista, un byte listo = una lectura, sin umbrales */
  uart_set_fifo_enabled(UART_ID, false);
}

int
main()
{
  //Llamamos a la configuracion inicial
  secure_boot_init();
  //Damos un segundo para que la terminal serial conecte antes de imprimir
  sleep_ms(1000);
  
  printf("\n === MODULO DE ARRANQUE SEGURO ===\n");
  printf("Sistema en modo bloqueo. Esperando clave de %d bytes...\n", KEY_LENGTH);
  printf("Ingresa los caracteres en la terminal:\n >");
  
  /* ===============================================================
     TODO 5:
     Logica principal de recepcion mediante Polling

     Estructura de dos bucles anidados:
     - Bucle exterior (while bytes_read < KEY_LENGTH):
         Mantiene al programa capturando hasta completar los 16 bytes.
         El CPU no puede abandonar este punto; su unica tarea es
         esta recepcion, exactamente como describia el correo.

       - Bucle interior (!uart_is_readable):
         Es el nucleo del Polling, uart_is_readable() consulta el
         bit RXFE (RX FIFO Empty) del registro de estado del UART
         Mientras el bit indique "sin datos", se ejecuta
         tight_loop_contents(), una instruccion NOP de ARM que evita
         efectos secundarios indeseados durante la espera activa.

       Cuando el byte esta listo (uart_is_readable devuelve true),
       uart_getc() lo lee del registro de datos del periferico y
       lo almacena en challenge_key. El printf del eco permite
       confirmar en la terminal del simulador que cada caracter
       fue recibido correctamente por el driver.
     =============================================================== */
  
  int bytes_read = 0;
  //Esperamos hasta tener exactamente 16 bytes
  while (bytes_read < KEY_LENGTH) {
    /* Busy Waiting:
       El CPU interroga al hardware en cada ciclo preguntando
       si ya hay un dato listo en el registro RX del UART */
    while (!uart_is_readable(UART_ID))
      tight_loop_contents(); //NOP de ARM para no quemar el CPU innecesariamente
    
    //Si salimos del bucle interior, significa que hay un byte listo
    challenge_key[bytes_read] = uart_getc(UART_ID); //Lo leemos y lo guardamos
    
    //Hacemos un 'eco' hacia la terminal para confirmar la recepcion
    printf("%c", challenge_key[bytes_read]); 
    bytes_read++;
  }
  
  key_received = true;//Confirmamos que terminamos
  
  // --- EL CODIGO TERMINA AQUI ---
  //Aseguramos que el arreglo sea un string valido en C para poder imprimirlo
  challenge_key[KEY_LENGTH] = '\0';

  //Si el flujo del programa llega a esta linea, la recepcion termino
  printf("\n \n[EXITO] Clave recibida: %s\n", challenge_key);
  printf("[SISTEMA] Validando integridad...\n");
  printf("[SISTEMA] Levantando RTOS...\n");
 
  //Bucle infinito simulando el ciclo de vida normal del sistema operativo
  while(1)
    tight_loop_contents();
  
  return 0;
}
