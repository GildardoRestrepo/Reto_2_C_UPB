/**
 * @file    gpio.h
 * @brief   Capa minima de acceso a GPIO por registros (bare-metal, sin HAL/LL).
 *
 * Solo se usan las definiciones de registros del fabricante (CMSIS):
 *   RCC->AHB1ENR, GPIOx->MODER, ->OTYPER, ->OSPEEDR, ->PUPDR, ->IDR, ->BSRR.
 *
 * El detalle de bits de cada registro esta documentado en _docs/registers.md.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include "stm32f4xx.h"

/** Valores del campo MODER (2 bits por pin). */
typedef enum {
    GPIO_MODE_INPUT     = 0u,   /**< 00: entrada                        */
    GPIO_MODE_OUTPUT    = 1u,   /**< 01: salida de proposito general    */
    GPIO_MODE_ALTERNATE = 2u,   /**< 10: funcion alternativa            */
    GPIO_MODE_ANALOG    = 3u    /**< 11: analogico                      */
} GpioMode_t;

/** Valores del campo OTYPER (1 bit por pin). */
typedef enum {
    GPIO_OTYPE_PUSHPULL  = 0u,  /**< 0: push-pull, empuja alto y bajo   */
    GPIO_OTYPE_OPENDRAIN = 1u   /**< 1: open-drain, solo tira a masa    */
} GpioOutType_t;

/** Valores del campo PUPDR (2 bits por pin). */
typedef enum {
    GPIO_PULL_NONE = 0u,        /**< 00: sin resistencia interna        */
    GPIO_PULL_UP   = 1u,        /**< 01: pull-up                        */
    GPIO_PULL_DOWN = 2u         /**< 10: pull-down                      */
} GpioPull_t;

/** Valores del campo OSPEEDR (2 bits por pin). */
typedef enum {
    GPIO_SPEED_LOW      = 0u,
    GPIO_SPEED_MEDIUM   = 1u,
    GPIO_SPEED_HIGH     = 2u,
    GPIO_SPEED_VERYHIGH = 3u
} GpioSpeed_t;

/**
 * @brief Habilita el reloj del puerto en RCC->AHB1ENR.
 *
 * CRITICO: hay que llamarla ANTES de escribir cualquier registro del puerto.
 * Tras el reset todos los relojes de periferico estan apagados, y escribir en
 * un GPIO sin reloj no produce ningun error: la escritura simplemente se pierde
 * en silencio. Es el fallo mas comun al empezar en bare-metal.
 *
 * @param port GPIOA..GPIOE. Cualquier otro valor se ignora.
 */
void gpio_enable_port_clock(GPIO_TypeDef *port);

/**
 * @brief Configura un pin suelto (modo, tipo de salida, pull y velocidad).
 * @param pin Numero de pin dentro del puerto, 0..15.
 */
void gpio_config_pin(GPIO_TypeDef *port,
                     uint32_t      pin,
                     GpioMode_t    mode,
                     GpioOutType_t otype,
                     GpioPull_t    pull,
                     GpioSpeed_t   speed);

/**
 * @brief Configura @p count pines contiguos a partir de @p first_pin.
 *
 * Es la funcion que usan los cuatro buses del proyecto (filas y columnas del
 * teclado y de la matriz), que por diseno son grupos contiguos dentro de su
 * puerto.
 */
void gpio_config_group(GPIO_TypeDef *port,
                       uint32_t      first_pin,
                       uint32_t      count,
                       GpioMode_t    mode,
                       GpioOutType_t otype,
                       GpioPull_t    pull,
                       GpioSpeed_t   speed);

/**
 * @brief Escribe @p value solo en los pines de @p mask, en UNA sola operacion.
 *
 * CRITICO: esta es la primitiva sobre la que se apoyan los dos multiplexados.
 *
 * BSRR codifica "poner a 1" en los bits 0..15 y "poner a 0" en los bits 16..31,
 * asi que una unica escritura cambia todo el bus a la vez. Frente a un
 * "ODR |= x" esto aporta tres cosas:
 *
 *   1. Es atomico: no hay ciclo leer-modificar-escribir que una interrupcion
 *      pueda partir por la mitad.
 *   2. No toca los demas pines del puerto, aunque no se conozca su estado.
 *   3. Permite SUBIR unos pines y BAJAR otros en la misma instruccion. En
 *      multiplexacion eso evita estados intermedios visibles (ghosting).
 *
 * @param mask  Pines sobre los que se actua.
 * @param value Nivel deseado para esos pines (los bits fuera de mask se ignoran).
 */
static inline void gpio_write_masked(GPIO_TypeDef *port, uint32_t mask, uint32_t value)
{
    const uint32_t bits_to_set = mask & value;
    const uint32_t bits_to_clr = mask & ~value;

    port->BSRR = (bits_to_clr << 16u) | bits_to_set;
}

/**
 * @brief Lee el estado real de los pines de @p mask.
 *
 * Se lee de IDR, nunca de ODR: IDR refleja el nivel FISICO del pin, mientras
 * que ODR solo devuelve lo ultimo que se escribio. Para leer el teclado la
 * diferencia es total.
 */
static inline uint32_t gpio_read_masked(const GPIO_TypeDef *port, uint32_t mask)
{
    return (port->IDR & mask);
}

#endif /* GPIO_H */
