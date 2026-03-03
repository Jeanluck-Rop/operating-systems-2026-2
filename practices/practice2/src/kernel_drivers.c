#include <stdint.h>

/* SIO base address for GPIO control on the RP2040 */
#define SIO_BASE 0xd0000000
/* SIO adresses for GPIO read operation */
#define SIO_GPIO_IN (SIO_BASE + 0x004)
/* SIO addresses for GPIO atomic set operation */
#define SIO_GPIO_OUT_SET (SIO_BASE + 0x014)
/* SIO addresses for GPIO atomic clear operation */
#define SIO_GPIO_OUT_CLR (SIO_BASE + 0x018)
/* SIO addresses for GPIO output enable set */
#define SIO_GPIO_OE_SET (SIO_BASE + 0x024)
/* SIO addresses for GPIO output enable clear */
#define SIO_GPIO_OE_CLR (SIO_BASE + 0x028)

/* IO bank 0 base address */
#define IO_BANK0_BASE 0x40014000
/* Offset for GPIO control registers */
#define IO_BANK0_GPIO_CTRL(pin) (IO_BANK0_BASE + 0x004 + (pin) * 8)
/* Function select for SIO (func 5) src/*/
#define GPIO_FUNC_SIO 5


//Pad control registers set electrical properties of each GPIO pin
//  such as drive strength, slew rate, and pull-up/pull-down resistors

//Base address of the PADS_BANK0 peripheral on the RP2040.
#define PADS_BANK0_BASE 0x4001C000
//Computes the address of the pad control register for a given pin
#define PADS_BANK0_GPIO(pin) (PADS_BANK0_BASE + 0x004 + (pin) * 4)
//Pull-Up Enable, setting this bit connects an internal resistor between the pin
//  and 3.3V, ensuring the pin reads 1 (high) when nothing is driving it
#define PADS_PUE (1 << 3)
//Pull-Down Enable
#define PADS_PDE (1 << 2)


/**
 * @brief Initialize GPIO pin as input or output and set function to SIO (func 5)
 * @param pin GPIO pin number
 * @param output 1 for output, 0 for input
 */
void k_gpio_init(uint32_t pin, uint32_t output)
{
  /* Select SIO function (func 5) for the pin */
  volatile uint32_t *gpio_ctrl = (volatile uint32_t *)IO_BANK0_GPIO_CTRL(pin);
  *gpio_ctrl = (*gpio_ctrl & ~0x1F) | GPIO_FUNC_SIO;

  /* Configure direction (output/input) */
  if (output)
    *(volatile uint32_t *)SIO_GPIO_OE_SET = (1 << pin);
  else {
    *(volatile uint32_t *)SIO_GPIO_OE_CLR = (1 << pin);

    //Get a pointer to this pin's pad control register
    volatile uint32_t *pad = (volatile uint32_t *)PADS_BANK0_GPIO(pin);
    
    //Enable the internal pull-up and explicitly disable the pull-down (PDE=0)
    //Both bits must be set correctly, because activating pull-up alone while pull-down remains
    //  enabled causes them to conflict, leaving the pin in an indeterminate floating state
    *pad = (*pad | PADS_PUE) & ~PADS_PDE;
  }
}

/**
 * @brief Set GPIO pin output value
 * @param pin GPIO pin number
 * @param value 1 for high, 0 for low
 */
void k_gpio_set(uint32_t pin, uint32_t value)
{
  //Writing a bitmask of (1 << pin) to SET drives the pin high,
  //  writing the same mask to CLR drives it low
  //This avoids race conditions if an interrupt fires between the read and the write
  if (value)
    *(volatile uint32_t *)SIO_GPIO_OUT_SET = (1 << pin);
  else
    *(volatile uint32_t *)SIO_GPIO_OUT_CLR = (1 << pin);
}

/**
 * @brief Get GPIO pin input value
 * @param pin GPIO pin number
 * @return 1 if high, 0 if low
 */
int k_gpio_get(uint32_t pin)
{
  //SIO_GPIO_IN holds the live state of all 30 GPIO pins simultaneously, one bit per pin
  //A volatile pointer forces the compiler to re-read the register
  //  on every access instead of caching a stale value
  volatile uint32_t *GPIO_INPUT_STATE = (volatile uint32_t *)SIO_GPIO_IN;

  //Shift the register right by 'pin' positions to move the target bit
  //  into bit 0, then mask with 1 to discard all other pins
  //Ex: pin = 2, register = 0b...100 -> >> 2 -> 0b...1 -> & 1 -> 1
  return (int)((*GPIO_INPUT_STATE >> pin) & 1);
}
