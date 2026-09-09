/**
 * @file    keypad.h
 * @brief   Driver del teclado matricial 4x4 con multiplexacion IN-OUT.
 *
 * Filas como salida open-drain, columnas como entrada con pull-up interno.
 * El barrido activa una fila por vez y lee las cuatro columnas.
 *
 * NIVEL DE ABSTRACCION: este modulo es hardware puro. Publica que tecla esta
 * pulsada AHORA, sin filtrar rebotes y sin saber que significa cada tecla.
 *   - El antirrebote lo anade keypad_fsm en la Fase 4.
 *   - La traduccion de indice a caracter ('1', 'A', '*'...) es semantica de
 *     aplicacion y vive en la Fase 5.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>

/** Valor devuelto cuando no hay ninguna tecla pulsada. */
#define KEYPAD_NO_KEY   0xFFu

/*
 * Distribucion fisica del teclado y los indices que devuelve este modulo:
 *
 *      [1][2][3][A]          0  1  2  3
 *      [4][5][6][B]          4  5  6  7
 *      [7][8][9][C]          8  9 10 11
 *      [*][0][#][D]         12 13 14 15
 *
 * El driver devuelve el INDICE, nunca el caracter. La tabla que traduce
 * indice -> caracter es semantica de aplicacion y vive en el modulo password
 * (Fase 5). Se documenta aqui para no perder el dato, no para usarlo aqui.
 */

/**
 * @brief Configura los 8 pines del teclado y deja todas las filas inactivas.
 *
 * Debe llamarse una sola vez, antes de entrar al while(1).
 */
void keypad_init(void);

/**
 * @brief Ejecuta un paso de la MEF de barrido.
 *
 * Debe llamarse exactamente una vez por tick de 1 ms desde el while(1).
 * Cada fila consume dos ticks (activar y leer), asi que un barrido completo de
 * las cuatro filas tarda 8 ms.
 */
void keypad_scan_step(void);

/**
 * @brief Indica si hay una instantanea NUEVA sin consumir, y la consume.
 *
 * Analoga a timebase_tick_ready(), pero al ritmo del barrido en vez del tick.
 *
 * Existe para la MEF de antirrebote: si esa maquina avanzara una vez por tick
 * muestrearia ocho veces seguidas el mismo valor, y su conteo de muestras
 * dejaria de equivaler a tiempo. Consumiendo esta bandera avanza una vez por
 * barrido, es decir cada 8 ms.
 *
 * @return 1 si habia instantanea nueva (y queda consumida), 0 si no.
 */
uint8_t keypad_snapshot_ready(void);

/**
 * @brief Ultima instantanea completa del teclado.
 *
 * @return Indice de tecla `fila * 4 + columna`, de 0 a 15, o KEYPAD_NO_KEY.
 *
 * El valor se actualiza al terminar cada barrido, es decir cada 8 ms.
 *
 * SIN ANTIRREBOTE: durante los primeros milisegundos de una pulsacion este
 * valor puede oscilar entre la tecla y KEYPAD_NO_KEY. Filtrarlo es trabajo de
 * la MEF de antirrebote, no de aqui.
 *
 * Si hay varias teclas pulsadas se devuelve la primera en orden de barrido.
 */
uint8_t keypad_get_raw_key(void);

#endif /* KEYPAD_H */
