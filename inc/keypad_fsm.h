/**
 * @file    keypad_fsm.h
 * @brief   MEF de antirrebote del teclado y contrato de eventos.
 *
 * Una tecla mecanica no genera una unica transicion limpia: el contacto abre y
 * cierra decenas de veces en unos milisegundos. Esta maquina convierte ese
 * flujo sucio en EVENTOS, con la garantia de que **cada pulsacion produce
 * exactamente uno**, mantenga el usuario la tecla el tiempo que la mantenga.
 *
 * Esta es la frontera entre el hardware y la aplicacion: aguas arriba se habla
 * de pines y niveles, aguas abajo solo de eventos.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#ifndef KEYPAD_FSM_H
#define KEYPAD_FSM_H

#include <stdint.h>

/** Tipo de evento publicado por la maquina de antirrebote. */
typedef enum {
    KEY_EVT_NONE = 0,   /**< No hay novedad.                         */
    KEY_EVT_PRESSED,    /**< Pulsacion validada. Una sola vez.       */
    KEY_EVT_RELEASED    /**< Liberacion validada. Una sola vez.      */
} KeyEventType_t;

/**
 * Evento del teclado.
 *
 * Es TODO lo que la capa de aplicacion conoce del teclado. No hay forma de
 * consultar pines, ni filas, ni el estado interno de la maquina: esa es la
 * separacion que pide el enunciado en su apartado 17.
 */
typedef struct {
    KeyEventType_t type;
    uint8_t        key;   /**< Indice 0..15. Valido si type no es NONE. */
} KeyEvent_t;

/** @brief Deja la maquina en reposo, sin evento pendiente. */
void keypad_fsm_init(void);

/**
 * @brief Ejecuta un paso de la MEF de antirrebote.
 *
 * Se llama una vez por tick desde el while(1), pero solo avanza cuando el
 * driver publica una instantanea nueva, es decir cada 8 ms. Asi el conteo de
 * muestras equivale directamente a tiempo.
 */
void keypad_fsm_step(void);

/**
 * @brief Recoge el evento pendiente y lo CONSUME.
 *
 * Al leerlo queda limpio, de modo que una pulsacion mantenida no puede generar
 * un segundo evento aunque nadie se acuerde de filtrar.
 *
 * @return El evento, o uno de tipo KEY_EVT_NONE si no habia novedad.
 */
KeyEvent_t keypad_fsm_get_event(void);

#endif /* KEYPAD_FSM_H */
