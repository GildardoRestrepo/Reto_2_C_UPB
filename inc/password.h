/**
 * @file    password.h
 * @brief   Contrasena almacenada, buffer de ingreso y comparacion.
 *
 * Este modulo es la SEMANTICA del teclado: aqui es donde un indice de tecla
 * pasa a significar un digito. El driver (keypad) no sabe nada de esto.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#ifndef PASSWORD_H
#define PASSWORD_H

#include <stdint.h>

/** Numero de digitos de la contrasena. */
#define PASSWORD_LENGTH     4u

/** Devuelto cuando una tecla no corresponde a ningun digito. */
#define PASSWORD_NO_DIGIT   0xFFu

/**
 * @brief Traduce un indice de tecla a su digito.
 *
 * Segun la serigrafia del teclado:
 *
 *      [1][2][3][A]          0  1  2  3
 *      [4][5][6][B]          4  5  6  7
 *      [7][8][9][C]          8  9 10 11
 *      [*][0][#][D]         12 13 14 15
 *
 * @return Digito 0..9, o PASSWORD_NO_DIGIT si la tecla no es numerica.
 */
uint8_t password_key_to_digit(uint8_t key);

/** @brief 1 si la tecla es la de borrado (`*`). */
uint8_t password_key_is_clear(uint8_t key);

/** @brief Vacia el buffer de ingreso. */
void password_clear(void);

/**
 * @brief Anade un digito al buffer de ingreso.
 * @return 1 si se acepto, 0 si el buffer ya estaba lleno.
 */
uint8_t password_add_digit(uint8_t digit);

/** @brief Cuantos digitos lleva ingresados el usuario, 0..PASSWORD_LENGTH. */
uint8_t password_count(void);

/** @brief 1 si ya se ingresaron los PASSWORD_LENGTH digitos. */
uint8_t password_is_complete(void);

/**
 * @brief Compara el buffer de ingreso con la contrasena guardada.
 *
 * @return 1 si coinciden, 0 si no.
 *
 * La comparacion se escribe a mano: el proyecto no usa `string.h`, asi que no
 * hay `memcmp`.
 */
uint8_t password_matches(void);

#endif /* PASSWORD_H */
