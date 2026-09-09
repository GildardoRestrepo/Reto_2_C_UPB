/**
 * @file    keypad_fsm.c
 * @brief   Implementacion de la MEF de antirrebote.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include "keypad_fsm.h"
#include "keypad.h"

/**
 * Muestras consecutivas iguales necesarias para dar por buena una transicion.
 *
 * Cada muestra es una instantanea del barrido, separada 8 ms de la anterior.
 * Cuatro muestras son por tanto TRES intervalos: 24 ms de estabilidad
 * confirmada, holgadamente por encima de los 5-20 ms que rebota un contacto
 * mecanico tipico.
 *
 * Si el teclado resultara rebotar mas, este es el unico numero que hay que
 * tocar. Por eso tiene nombre y no esta escrito suelto en el codigo.
 */
#define KEYPAD_DEBOUNCE_SAMPLES     4u

/** Estados de la maquina. Ver el diagrama en _docs/architecture.md, seccion 7. */
typedef enum {
    KEY_IDLE = 0u,          /**< Sin tecla. Esperando una lectura distinta de NINGUNA. */
    KEY_DEBOUNCE_PRESS,     /**< Confirmando una pulsacion.                            */
    KEY_PRESSED,            /**< Estado DE PASO: emite KEY_EVT_PRESSED.                */
    KEY_HELD,               /**< Tecla mantenida. No se emite nada.                    */
    KEY_DEBOUNCE_RELEASE    /**< Confirmando la liberacion.                            */
} DebounceState_t;

static DebounceState_t s_state;

/** Tecla que se esta confirmando durante KEY_DEBOUNCE_PRESS. */
static uint8_t s_candidate;

/** Tecla ya confirmada, la que se considera pulsada. */
static uint8_t s_held_key;

/** Muestras iguales acumuladas en el estado actual. */
static uint8_t s_samples;

/** Evento pendiente de que alguien lo recoja. */
static KeyEvent_t s_event;

void keypad_fsm_init(void)
{
    s_state      = KEY_IDLE;
    s_candidate  = KEYPAD_NO_KEY;
    s_held_key   = KEYPAD_NO_KEY;
    s_samples    = 0u;
    s_event.type = KEY_EVT_NONE;
    s_event.key  = KEYPAD_NO_KEY;
}

void keypad_fsm_step(void)
{
    /* CRITICO: la maquina avanza al ritmo del BARRIDO, no del tick.
     *
     * El driver publica una instantanea cada 8 ms. Si esta funcion avanzara en
     * cada tick de 1 ms, leeria ocho veces seguidas exactamente el mismo valor
     * y las "cuatro muestras estables" serian 4 ms en lugar de 24: no filtraria
     * nada. Consumiendo la bandera del driver, cada muestra es una lectura
     * genuinamente nueva. */
    if (keypad_snapshot_ready() == 0u) {
        return;
    }

    const uint8_t reading = keypad_get_raw_key();

    switch (s_state) {

    case KEY_IDLE:
        if (reading != KEYPAD_NO_KEY) {
            s_candidate = reading;
            s_samples   = 1u;
            s_state     = KEY_DEBOUNCE_PRESS;
        }
        break;

    case KEY_DEBOUNCE_PRESS:
        if (reading != s_candidate) {
            /* La lectura no se sostuvo: era rebote, o el usuario solto antes de
             * tiempo. Se descarta sin emitir nada. */
            s_state = KEY_IDLE;
        } else {
            s_samples++;
            if (s_samples >= KEYPAD_DEBOUNCE_SAMPLES) {
                s_state = KEY_PRESSED;
            }
        }
        break;

    case KEY_HELD:
        /* Cualquier lectura distinta arranca la confirmacion de suelta. Eso
         * incluye que el usuario pulse una SEGUNDA tecla sin soltar la primera:
         * se trata como una liberacion, de modo que la segunda tecla tenga que
         * pasar por KEY_IDLE y su propio antirrebote para ser valida. */
        if (reading != s_held_key) {
            s_samples = 1u;
            s_state   = KEY_DEBOUNCE_RELEASE;
        }
        break;

    case KEY_DEBOUNCE_RELEASE:
        if (reading == s_held_key) {
            /* Reapareció la tecla: era rebote de la suelta. */
            s_state = KEY_HELD;
        } else {
            s_samples++;
            if (s_samples >= KEYPAD_DEBOUNCE_SAMPLES) {
                s_event.type = KEY_EVT_RELEASED;
                s_event.key  = s_held_key;
                s_held_key   = KEYPAD_NO_KEY;
                s_state      = KEY_IDLE;
            }
        }
        break;

    case KEY_PRESSED:
    default:
        /* Inalcanzable: KEY_PRESSED se atraviesa mas abajo, dentro del mismo
         * paso en que se alcanza, y nunca sobrevive hasta el siguiente. */
        s_state = KEY_IDLE;
        break;
    }

    /* CRITICO: KEY_PRESSED es un estado DE PASO.
     *
     * Se atraviesa dentro del mismo paso en que se alcanza y es el UNICO punto
     * del programa que emite KEY_EVT_PRESSED. Que una pulsacion mantenida no
     * pueda generar cientos de eventos no depende de que la capa de aplicacion
     * se acuerde de filtrar: es estructural, no hay ningun camino en la maquina
     * que vuelva a pasar por aqui sin haber pasado antes por KEY_IDLE. */
    if (s_state == KEY_PRESSED) {
        s_held_key   = s_candidate;
        s_event.type = KEY_EVT_PRESSED;
        s_event.key  = s_held_key;
        s_state      = KEY_HELD;
    }
}

KeyEvent_t keypad_fsm_get_event(void)
{
    const KeyEvent_t event = s_event;

    /* Se consume al leerlo. Si nadie lo recoge antes del siguiente evento, el
     * nuevo lo sobrescribe; con el bucle principal llamando cada tick eso no
     * puede ocurrir, porque entre dos eventos median al menos 24 ms. */
    s_event.type = KEY_EVT_NONE;
    s_event.key  = KEYPAD_NO_KEY;

    return event;
}
