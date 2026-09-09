/**
 * @file    images.c
 * @brief   Definicion de los mapas de bits de 8x8.
 *
 * Los arreglos se escriben en binario a proposito: asi el codigo fuente es el
 * dibujo. Ver el convenio de filas y columnas en images.h.
 *
 * Van declarados `const` para que el enlazador los coloque en FLASH y no
 * consuman RAM.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include "images.h"

/* Solo la fila superior. Detecta el intercambio de filas por columnas. */
const uint8_t img_test_row0[8] =
{
    0b11111111,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
};

/* Letra F. Detecta espejados y giros: no coincide consigo misma bajo ninguna
 * transformacion, asi que basta mirarla para saber que corregir. */
const uint8_t img_test_f[8] =
{
    0b11111110,
    0b10000000,
    0b10000000,
    0b11111000,
    0b10000000,
    0b10000000,
    0b10000000,
    0b00000000
};

/* Marco de un pixel. Ejercita las 8 filas y las 8 columnas. */
const uint8_t img_test_border[8] =
{
    0b11111111,
    0b10000001,
    0b10000001,
    0b10000001,
    0b10000001,
    0b10000001,
    0b10000001,
    0b11111111
};

/* Los 64 LED encendidos. Prueba decisiva para localizar filas o columnas
 * muertas: aparecen como lineas oscuras. */
const uint8_t img_test_all[8] =
{
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111
};

/* Reposo: solo las cuatro esquinas. Confirma que el sistema sigue vivo cuando
 * no hay ninguna tecla pulsada. */
const uint8_t img_idle_corners[8] =
{
    0b10000001,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b10000001
};
