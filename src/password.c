/**
 * @file    password.c
 * @brief   Implementacion del almacenamiento y la validacion de la contrasena.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include "password.h"

/** Indice de la tecla `*`, la de borrado, segun la serigrafia del teclado. */
#define KEY_INDEX_CLEAR     12u

/** Numero de teclas del teclado matricial. */
#define KEYPAD_KEY_COUNT    16u

/**
 * CONTRASENA GUARDADA: 8 1 9 1
 *
 * DECISION DE DISENO: se declara `const`, de modo que el enlazador la coloca en
 * FLASH junto al codigo y no en RAM.
 *
 * Ventaja: ningun fallo de software puede alterarla en ejecucion; no hay
 * puntero que pueda pisarla ni desbordamiento de pila que la corrompa.
 * Coste: cambiarla exige recompilar y reflashear la placa.
 *
 * El enunciado no pide cifrado, y no lo hay: cualquiera con el .hex puede
 * leerla. Es una limitacion consciente y documentada, no un descuido.
 *
 * Para cambiar la clave, esta linea es lo unico que hay que tocar.
 */
static const uint8_t k_stored_password[PASSWORD_LENGTH] = { 8u, 1u, 9u, 1u };

/**
 * Traduccion de indice de tecla a digito.
 *
 * Va en FLASH igual que la contrasena. Una tabla de 16 bytes resuelve el mapeo
 * sin un solo `if`, y si algun dia cambia la serigrafia del teclado, o si al
 * cablear resultan cruzadas las filas y las columnas, se corrige reordenando
 * esta tabla y nada mas.
 */
static const uint8_t k_key_to_digit[KEYPAD_KEY_COUNT] = {
    /*  0 -> '1' */ 1u,
    /*  1 -> '2' */ 2u,
    /*  2 -> '3' */ 3u,
    /*  3 -> 'A' */ PASSWORD_NO_DIGIT,
    /*  4 -> '4' */ 4u,
    /*  5 -> '5' */ 5u,
    /*  6 -> '6' */ 6u,
    /*  7 -> 'B' */ PASSWORD_NO_DIGIT,
    /*  8 -> '7' */ 7u,
    /*  9 -> '8' */ 8u,
    /* 10 -> '9' */ 9u,
    /* 11 -> 'C' */ PASSWORD_NO_DIGIT,
    /* 12 -> '*' */ PASSWORD_NO_DIGIT,
    /* 13 -> '0' */ 0u,
    /* 14 -> '#' */ PASSWORD_NO_DIGIT,
    /* 15 -> 'D' */ PASSWORD_NO_DIGIT
};

/** Buffer de lo que el usuario lleva tecleado. En RAM, obviamente. */
static uint8_t s_entry[PASSWORD_LENGTH];

/** Cuantas posiciones del buffer estan ocupadas. */
static uint8_t s_count;

uint8_t password_key_to_digit(uint8_t key)
{
    if (key >= KEYPAD_KEY_COUNT) {
        return PASSWORD_NO_DIGIT;
    }

    return k_key_to_digit[key];
}

uint8_t password_key_is_clear(uint8_t key)
{
    return (key == KEY_INDEX_CLEAR) ? 1u : 0u;
}

void password_clear(void)
{
    for (uint8_t i = 0u; i < PASSWORD_LENGTH; i++) {
        s_entry[i] = 0u;
    }

    s_count = 0u;
}

uint8_t password_add_digit(uint8_t digit)
{
    if (s_count >= PASSWORD_LENGTH) {
        return 0u;
    }

    s_entry[s_count] = digit;
    s_count++;

    return 1u;
}

uint8_t password_count(void)
{
    return s_count;
}

uint8_t password_is_complete(void)
{
    return (s_count >= PASSWORD_LENGTH) ? 1u : 0u;
}

uint8_t password_matches(void)
{
    if (s_count != PASSWORD_LENGTH) {
        return 0u;
    }

    /* Comparacion escrita a mano: sin `string.h` no hay memcmp.
     *
     * Se recorren siempre las cuatro posiciones, sin salir al primer fallo. No
     * es por rendimiento (es irrelevante), sino porque el tiempo de ejecucion
     * no depende de cuantos digitos acertados haya: un `return` anticipado
     * filtraria informacion por temporizacion. Aqui no hay atacante midiendo
     * microsegundos, pero es la forma correcta de escribirlo y sale gratis. */
    uint8_t equal = 1u;

    for (uint8_t i = 0u; i < PASSWORD_LENGTH; i++) {
        if (s_entry[i] != k_stored_password[i]) {
            equal = 0u;
        }
    }

    return equal;
}
