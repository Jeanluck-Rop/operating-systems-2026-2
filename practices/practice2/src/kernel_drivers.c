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

/**/
#define PADS_BANK0_BASE 0x4001C000
#define PADS_BANK0_GPIO(pin) (PADS_BANK0_BASE + 0x004 + (pin) * 4)
#define PADS_PUE (1 << 3)
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
    volatile uint32_t *pad = (volatile uint32_t *)PADS_BANK0_GPIO(pin);
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
  // TODO: Use volatile uint32_t pointers to access SIO registers
  // Depending on value, set or clear the pin atomically (1 << pin) bitmask
  if (value)
    *(volatile uint32_t *)SIO_GPIO_OUT_SET = (1u << pin);
  else
    *(volatile uint32_t *)SIO_GPIO_OUT_CLR = (1u << pin);
}

/**
 * @brief Get GPIO pin input value
 * @param pin GPIO pin number
 * @return 1 if high, 0 if low
 */
int k_gpio_get(uint32_t pin)
{
  // TODO: Define a volatile uint32_t pointer to read the GPIO input state
  // Return the state of the specified pin by shifting and masking
  volatile uint32_t *GPIO_INPUT_STATE = (volatile uint32_t *)SIO_GPIO_IN;
  return (int)((*GPIO_INPUT_STATE >> pin) & 1);
}
