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

/* --------------------------------------------------------------------------
 * Digitos 0-9, tipografia de 5x7.
 *
 * Escritos en binario, como el resto: el codigo fuente ES el dibujo, y ajustar
 * un trazo que no se lee bien en la matriz es mover un bit.
 * -------------------------------------------------------------------------- */
const uint8_t img_digit[10][8] =
{
    {   /* 0 */
        0b00111000,
        0b01000100,
        0b01001100,
        0b01010100,
        0b01100100,
        0b01000100,
        0b00111000,
        0b00000000
    },
    {   /* 1 */
        0b00010000,
        0b00110000,
        0b00010000,
        0b00010000,
        0b00010000,
        0b00010000,
        0b00111000,
        0b00000000
    },
    {   /* 2 */
        0b00111000,
        0b01000100,
        0b00000100,
        0b00001000,
        0b00010000,
        0b00100000,
        0b01111100,
        0b00000000
    },
    {   /* 3 */
        0b01111100,
        0b00001000,
        0b00010000,
        0b00001000,
        0b00000100,
        0b01000100,
        0b00111000,
        0b00000000
    },
    {   /* 4 */
        0b00001000,
        0b00011000,
        0b00101000,
        0b01001000,
        0b01111100,
        0b00001000,
        0b00001000,
        0b00000000
    },
    {   /* 5 */
        0b01111100,
        0b01000000,
        0b01111000,
        0b00000100,
        0b00000100,
        0b01000100,
        0b00111000,
        0b00000000
    },
    {   /* 6 */
        0b00011000,
        0b00100000,
        0b01000000,
        0b01111000,
        0b01000100,
        0b01000100,
        0b00111000,
        0b00000000
    },
    {   /* 7 */
        0b01111100,
        0b00000100,
        0b00001000,
        0b00010000,
        0b00100000,
        0b00100000,
        0b00100000,
        0b00000000
    },
    {   /* 8 */
        0b00111000,
        0b01000100,
        0b01000100,
        0b00111000,
        0b01000100,
        0b01000100,
        0b00111000,
        0b00000000
    },
    {   /* 9 */
        0b00111000,
        0b01000100,
        0b01000100,
        0b00111100,
        0b00000100,
        0b00001000,
        0b00110000,
        0b00000000
    }
};

/* Visto: ACCESO PERMITIDO. Trazo corto descendente que se une al largo
 * ascendente; grueso a proposito para que se lea de lejos. */
const uint8_t img_check[8] =
{
    0b00000000,
    0b00000001,
    0b00000011,
    0b00000110,
    0b10001100,
    0b11011000,
    0b01110000,
    0b00100000
};

/* Equis: ACCESO DENEGADO. Las dos diagonales completas de esquina a esquina:
 * imposible de confundir con ningun digito. */
const uint8_t img_cross[8] =
{
    0b10000001,
    0b01000010,
    0b00100100,
    0b00011000,
    0b00011000,
    0b00100100,
    0b01000010,
    0b10000001
};
