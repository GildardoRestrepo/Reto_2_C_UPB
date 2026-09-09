/**
 * @file    board.h
 * @brief   Mapa de pines y constantes de la placa STM32F407VET6 ("black board").
 *
 * PUNTO UNICO de configuracion de hardware. Si cambia el cableado se toca este
 * archivo y ningun otro: los drivers no contienen numeros de pin.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#ifndef BOARD_H
#define BOARD_H

#include "stm32f4xx.h"

/* ==========================================================================
 * Reloj del sistema
 * ==========================================================================
 * Se usa el HSI a 16 MHz, que es el reloj activo por defecto tras un reset.
 *
 * DECISION DE DISENO: no se configura el PLL. Evita tener que ajustar la
 * latencia de FLASH (FLASH->ACR) y toda la cadena RCC->PLLCFGR / RCC->CFGR,
 * y 16 MHz sobran para una base de tiempo de 1 ms.
 *
 * CRITICO: de esta constante depende la recarga de SysTick. Si algun dia se
 * activa el PLL, hay que actualizarla o la base de tiempo dejara de ser 1 ms.
 */
#define BOARD_SYSCLK_HZ             16000000uL

/* ==========================================================================
 * Matriz LED 8x8  -  multiplexacion OUT-OUT (filas y columnas son salidas)
 * ========================================================================== */

/** Columnas C1..C8 de la matriz -> PD0..PD7 (8 pines contiguos). */
#define MATRIX_COL_PORT             GPIOD
#define MATRIX_COL_FIRST_PIN        0u

/** Filas F1..F8 de la matriz -> PE8..PE15 (8 pines contiguos).
 *
 * MOTIVO DE LA ELECCION: en los conectores 2x24 de la placa, PE8..PE15 salen
 * como cuatro parejas seguidas (PE7 PE8 | PE9 PE10 | PE11 PE12 | PE13 PE14 |
 * PE15), asi que el bus se cablea sin saltos. Son lineas FSMC_D5..D12, que solo
 * se usan si se conecta un LCD por FSMC; en el puerto E hay que evitar PE3 y
 * PE4, que llevan los botones K0/K1 y el conector NRF24, y quedan lejos.
 *
 * Ademas PE8..PE15 es el BYTE ALTO del puerto E, igual que PD0..PD7 es el byte
 * bajo del D: el patron de fila se mapea con un simple desplazamiento. */
#define MATRIX_ROW_PORT             GPIOE
#define MATRIX_ROW_FIRST_PIN        8u

#define MATRIX_ROWS                 8u
#define MATRIX_COLS                 8u

/* ==========================================================================
 * Teclado matricial 4x4  -  multiplexacion IN-OUT
 * ==========================================================================
 * DECISION DE DISENO: filas como salida OPEN-DRAIN y columnas como entrada con
 * PULL-UP interno.
 *
 * Con open-drain, escribir 0 en una fila la lleva a masa (fila activa) y
 * escribir 1 la deja en ALTA IMPEDANCIA, no la sube a 3V3. Gracias a eso, si el
 * usuario pulsa dos teclas a la vez nunca se conectan entre si una salida en
 * alto y otra en bajo, que es lo que ocurriria con salidas push-pull.
 *
 * Logica ACTIVA EN BAJO: una columna leida a 0 significa tecla pulsada.
 */

/** Filas F0..F3 del teclado -> PB6..PB9 (salidas open-drain). */
#define KEYPAD_ROW_PORT             GPIOB
#define KEYPAD_ROW_FIRST_PIN        6u

/** Columnas C0..C3 del teclado -> PA1..PA4 (entradas con pull-up). */
#define KEYPAD_COL_PORT             GPIOA
#define KEYPAD_COL_FIRST_PIN        1u

#define KEYPAD_ROWS                 4u
#define KEYPAD_COLS                 4u

/* ==========================================================================
 * LED de diagnostico de la placa
 * ==========================================================================
 * D2 de la "black board": el anodo va a 3V3 a traves de R13 (510R) y el catodo
 * a PA6. Es decir, el pin SUMIDERO de corriente:
 *
 *      3V3 ---[510R]---|>|--- PA6
 *
 * Por tanto es ACTIVO EN BAJO: escribir 0 enciende, escribir 1 apaga.
 * Solo se usa para diagnostico (Fase 1); no forma parte del sistema final.
 */
#define DEBUG_LED_PORT              GPIOA
#define DEBUG_LED_PIN               6u
#define DEBUG_LED_ACTIVE_LOW        1

#endif /* BOARD_H */
