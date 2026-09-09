/**
 * @file    gpio.c
 * @brief   Implementacion de la capa de acceso a GPIO por registros.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include "gpio.h"

/* Los registros MODER, OSPEEDR y PUPDR dedican 2 bits a cada pin; OTYPER, 1. */
#define GPIO_FIELD_2BITS_MASK       0x3u
#define GPIO_FIELD_2BITS_WIDTH      2u

void gpio_enable_port_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    } else if (port == GPIOB) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    } else if (port == GPIOC) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    } else if (port == GPIOD) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    } else if (port == GPIOE) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
    } else {
        /* Puerto no utilizado en este proyecto: no se habilita nada. */
        return;
    }

    /* CRITICO: lectura de vuelta obligatoria.
     *
     * La habilitacion del reloj tarda unos ciclos en propagarse por el bus
     * AHB1. Sin esta lectura, la instruccion inmediatamente siguiente puede
     * intentar escribir un registro del puerto que todavia no responde, y esa
     * escritura se pierde sin aviso. Leer el registro fuerza al nucleo a
     * esperar a que el bus complete la transaccion. */
    (void)RCC->AHB1ENR;
}

void gpio_config_pin(GPIO_TypeDef *port,
                     uint32_t      pin,
                     GpioMode_t    mode,
                     GpioOutType_t otype,
                     GpioPull_t    pull,
                     GpioSpeed_t   speed)
{
    /* Desplazamiento del campo de este pin dentro de los registros de 2 bits. */
    const uint32_t shift_2b = pin * GPIO_FIELD_2BITS_WIDTH;

    /* CRITICO: en los cuatro registros se LIMPIA el campo antes de escribirlo.
     *
     * Un "|=" por si solo nunca puede cambiar un 1 a 0, asi que reconfigurar un
     * pin sin limpiar primero dejaria mezclados los bits del modo anterior con
     * los del nuevo. Limpiar y luego escribir garantiza que el campo queda
     * exactamente con el valor pedido, venga de donde venga. */

    /* MODER: entrada / salida / alternativa / analogico. */
    port->MODER &= ~(GPIO_FIELD_2BITS_MASK << shift_2b);
    port->MODER |=  ((uint32_t)mode << shift_2b);

    /* OTYPER: push-pull u open-drain. Un solo bit por pin. */
    port->OTYPER &= ~(1u << pin);
    port->OTYPER |=  ((uint32_t)otype << pin);

    /* OSPEEDR: velocidad de flanco. Baja es suficiente en este proyecto y
     * genera menos ruido conmutado en la protoboard. */
    port->OSPEEDR &= ~(GPIO_FIELD_2BITS_MASK << shift_2b);
    port->OSPEEDR |=  ((uint32_t)speed << shift_2b);

    /* PUPDR: resistencias internas de pull-up / pull-down. */
    port->PUPDR &= ~(GPIO_FIELD_2BITS_MASK << shift_2b);
    port->PUPDR |=  ((uint32_t)pull << shift_2b);
}

void gpio_config_group(GPIO_TypeDef *port,
                       uint32_t      first_pin,
                       uint32_t      count,
                       GpioMode_t    mode,
                       GpioOutType_t otype,
                       GpioPull_t    pull,
                       GpioSpeed_t   speed)
{
    for (uint32_t i = 0u; i < count; i++) {
        gpio_config_pin(port, first_pin + i, mode, otype, pull, speed);
    }
}
