#include <stdint.h>

// === MAPA DE MEMORIA (RP2040 Datasheet) ===
// 1. SUBSYSTEM RESETS (Base: 0x4000C000)
// Controla que perifericos estan despiertos o dormidos .
#define RESETS_BASE 0x4000C000
#define RESETS_RESET (*(volatile uint32_t *)(RESETS_BASE + 0x0))
#define RESETS_DONE (*(volatile uint32_t *)(RESETS_BASE + 0x8))

// 2. IO BANK 0 (Base: 0 x40014000)
// Controla la funcion logica de los pines (GPIO , UART , etc).
#define IO_BANK0_BASE 0x40014000
// El pin 25 tiene un offset de 0x0CC
#define GPIO25_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x0CC))

// 3. SINGLE - CYCLE IO (SIO) (Base: 0xD0000000)
// Controla el estado alto / bajo de los pines de forma rapida.
#define SIO_BASE 0xD0000000
#define SIO_GPIO_OE (*(volatile uint32_t *)(SIO_BASE + 0x020)) // Output Enable
#define SIO_GPIO_OUT (*(volatile uint32_t *)(SIO_BASE + 0x010)) // Output Value

// NOTA: Si usas Pico W, cambia 25 por un pin externo (ej . 16)
#define LED_PIN 25

//valores para los tiempos del LED prendido/apagado
const int START_LED_ON = 15000000;
const int START_LED_OFF = 5000000;
const int BIT_ON = 8000000;
const int BIT_OFF = 2500000;
const int DIGIT_DELAY = 6000000;

/* Funcion auxiliar para dar un retraso (o delay) forzado entre las acciones de prender y apagar el LED */
int
delay(int time)
{
  for (volatile int i = 0; i <= time; i ++);
}

/* Funcion principal para prender nuestro numero de cuenta en el LED */
int
main()
{
  // --- PASO 1: BOOTSTRAPPING (Despertar Hardware) ---
  // Debemos quitar el Reset a dos componentes:
  // Bit 5: IO_BANK0 (Logica de pines)
  // Bit 8: PADS_BANK0 (Electricidad de pines)
  // Escribimos 0 en esos bits para sacarlos del reset.
  RESETS_RESET &= ~((1 << 5) | (1 << 8) );
  // Esperamos hasta que el hardware confirme que desperto
  while((RESETS_DONE & ((1 << 5) | (1 << 8))) != ((1 << 5) | (1 << 8)));
  
  // --- PASO 2: CONFIGURACION ---
  // Seleccionamos la Funcion 5 (SIO - Software Control) para el Pin 25
  GPIO25_CTRL = 5;
  // Habilitamos el Pin 25 como SALIDA en el SIO
  SIO_GPIO_OE |= (1 << LED_PIN);

  // --- PASO 3: BUCLE INFINITO (Kernel Loop) ---
  while (1) {
    //damos un conteo regresivo para indicar que
    //se empezara a mostrar el numero de cuenta
    for (int i = 0; i < 3; i++) {
      SIO_GPIO_OUT |= (1 << LED_PIN); //prendemos el LED
      delay(START_LED_ON); //hacemos tiempo para que se note el conteo regresivo
      SIO_GPIO_OUT &= ~(1 << LED_PIN); //lo apagamos
      delay(START_LED_OFF); //damos otro tiempo de espera mas entre el conteo del LED
    }
    //el numero de cuenta a mostrar
    int no_cuenta[] = {1, 1, 8, 0, 0, 5, 7, 6, 2};
    //iteramos digito por digito en el numero de cuenta
    for (int d = 0; d < 9; d++) {
      //iterador para prender 4 bits y representar el digito en binario
      for (int bit = 0; bit < 4; bit++) {
	//utilizamos |= y &= ~ para apagar y/o prender los LEDS dependiendo del estado del bit
	if (no_cuenta[d] & (1 << bit))
	  SIO_GPIO_OUT |= (1 << LED_PIN); //si en la posicion del bit es 1, prendemos el LED
	else
	  SIO_GPIO_OUT &= ~(1 << LED_PIN); //en otro caso lo apagamos
	
	//dejamos el bit/led prendido o apagado por
	delay(BIT_ON);
	//apagamos el LED entre bits
	SIO_GPIO_OUT &= ~(1 << LED_PIN);
	//damos una espera antes de continuar con el siguiente bit
	delay(BIT_OFF);
      }
      //damos un intervalo entre digitos
      //como ya apagamos el bit en el 4 bit del for anterior, no es necesario volverlo a apagar
      delay(DIGIT_DELAY);
    }
  }
  
  return 0;
}
