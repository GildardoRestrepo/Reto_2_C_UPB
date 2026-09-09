/**
 * @file    images.h
 * @brief   Mapas de bits de 8x8 para la matriz LED.
 *
 * CONVENIO (unico para todo el proyecto):
 *   - `imagen[0]` es la fila SUPERIOR, `imagen[7]` la INFERIOR.
 *   - Dentro de cada byte, el **bit 7 es la columna IZQUIERDA** y el bit 0 la
 *     derecha.
 *
 * Escritos asi, los arreglos en binario se leen igual que se ven en la matriz,
 * que es lo que hace mantenibles los bitmaps. La traduccion al orden real de
 * los pines la hace led_matrix_show(), no el bitmap.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#ifndef IMAGES_H
#define IMAGES_H

#include <stdint.h>

/**
 * @brief Diagnostico 1: solo la fila superior encendida.
 *
 * Si en la matriz aparece como una COLUMNA en lugar de una fila, las filas y
 * las columnas estan intercambiadas en el cableado.
 */
extern const uint8_t img_test_row0[8];

/**
 * @brief Diagnostico 2: la letra F.
 *
 * Es la imagen mas util de las tres porque no coincide consigo misma bajo
 * ninguna transformacion: si sale espejada en horizontal, en vertical, girada o
 * transpuesta, se ve a simple vista cual de las cuatro cosas paso. Con eso se
 * ajustan MATRIX_ROW_REVERSE y MATRIX_COL_REVERSE en board.h.
 */
extern const uint8_t img_test_f[8];

/**
 * @brief Diagnostico 3: marco de un pixel.
 *
 * Enciende las cuatro esquinas y los cuatro bordes, de modo que ejercita las 8
 * filas y las 8 columnas. Un hilo suelto aparece como un tramo de borde que
 * falta, e identifica exactamente que pin revisar.
 */
extern const uint8_t img_test_border[8];

/**
 * @brief Diagnostico 4: los 64 LED encendidos.
 *
 * Es la prueba decisiva para localizar un pin muerto: una columna que no
 * responde aparece como una linea vertical oscura y una fila que no responde
 * como una horizontal. Identifica el pin exacto de un vistazo.
 *
 * No supone ningun esfuerzo electrico adicional: como el multiplexado enciende
 * una sola fila a la vez, el pin de fila conduce ocho LED igual que ya lo hace
 * con img_test_row0.
 */
extern const uint8_t img_test_all[8];

#endif /* IMAGES_H */
