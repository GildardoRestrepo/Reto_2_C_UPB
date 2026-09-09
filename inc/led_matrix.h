/**
 * @file    led_matrix.h
 * @brief   Driver de la matriz LED 8x8 con multiplexacion OUT-OUT.
 *
 * Filas y columnas son salidas digitales. La imagen completa se construye
 * activando UNA fila por tick de 1 ms: el cuadro entero se refresca cada 8 ms,
 * es decir a 125 Hz, muy por encima del umbral de parpadeo perceptible.
 *
 * La capa de aplicacion nunca escribe pines: entrega un arreglo de 8 bytes con
 * led_matrix_show() y se olvida. Quien traduce ese arreglo al orden real del
 * cableado es este modulo.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <stdint.h>

/**
 * @brief Configura los 16 pines de la matriz y deja el display apagado.
 *
 * Debe llamarse una sola vez, antes de entrar al while(1).
 */
void led_matrix_init(void);

/**
 * @brief Carga una imagen en el framebuffer.
 *
 * @param frame Arreglo de 8 bytes con el convenio de images.h: el byte 0 es la
 *              fila superior y el bit 7 de cada byte la columna izquierda.
 *
 * La traduccion al orden fisico de los pines (y las correcciones de espejado
 * de board.h) se aplican aqui, una sola vez por cambio de imagen, y no en el
 * multiplexado, que se ejecuta mil veces por segundo.
 */
void led_matrix_show(const uint8_t *frame);

/** @brief Apaga toda la matriz (framebuffer a cero). */
void led_matrix_clear(void);

/**
 * @brief Ejecuta un paso de la MEF de multiplexado: refresca UNA fila.
 *
 * Debe llamarse exactamente una vez por tick de 1 ms desde el while(1).
 * El estado de la maquina es la fila activa: un anillo de ocho estados que
 * avanza un paso en cada llamada.
 */
void led_matrix_mux_step(void);

#endif /* LED_MATRIX_H */
