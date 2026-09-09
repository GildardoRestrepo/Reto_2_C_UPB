/**
 * @file    system_fsm.c
 * @brief   Implementacion de la MEF de la aplicacion.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include "system_fsm.h"
#include "password.h"
#include "timebase.h"

/**
 * Tiempo que permanecen visibles las imagenes de acceso y de error.
 *
 * CRITICO: no es un retardo. Es un temporizador software; durante estos 3
 * segundos el sistema sigue multiplexando la matriz, barriendo el teclado y
 * ejecutando todas las maquinas de estados, tal y como exige el enunciado.
 */
#define RESULT_DISPLAY_MS       3000u

/** Intentos fallidos seguidos que disparan el bloqueo (Fase 7). */
#define MAX_FAILED_ATTEMPTS     3u

/** Estados. Ver el diagrama en _docs/architecture.md, seccion 9. */
typedef enum {
    ST_ESPERA = 0u,     /**< Sin nada tecleado.                              */
    ST_INGRESANDO,      /**< Recibiendo digitos.                             */
    ST_VALIDANDO,       /**< Estado DE PASO: compara y decide.               */
    ST_ACCESO,          /**< Mostrando acceso permitido durante 3 s.         */
    ST_ERROR            /**< Mostrando acceso denegado durante 3 s.          */
} SysState_t;

static SysState_t   s_state;
static SysDisplay_t s_display;
static SwTimer_t    s_result_timer;

/**
 * Fallos consecutivos. Se lleva desde ya, aunque el bloqueo sea de la Fase 7:
 * asi esa fase sera anadir un estado y no rehacer esta maquina.
 */
static uint8_t s_failed_attempts;

static void enter_espera(void);
static void handle_pressed(uint8_t key);

void system_fsm_init(void)
{
    s_failed_attempts = 0u;
    sw_timer_stop(&s_result_timer);
    enter_espera();
}

void system_fsm_step(KeyEvent_t event)
{
    switch (s_state) {

    case ST_ESPERA:
    case ST_INGRESANDO:
        if (event.type == KEY_EVT_PRESSED) {
            handle_pressed(event.key);

        } else if (event.type == KEY_EVT_RELEASED) {
            /* El enunciado lo pide explicitamente: al soltar la tecla, la
             * matriz vuelve al estado de espera o de ingreso. Se deja de
             * mostrar el digito y se pasa a la barra de progreso. */
            if (s_state == ST_INGRESANDO) {
                s_display.view  = SYS_VIEW_ENTERING;
                s_display.count = password_count();
            }

        } else {
            /* KEY_EVT_NONE: nada que hacer. */
        }
        break;

    case ST_ACCESO:
    case ST_ERROR:
        /* Las pulsaciones se ignoran mientras se muestra el resultado. Lo unico
         * que saca de aqui es que venza el temporizador. */
        if (sw_timer_expired(&s_result_timer) != 0u) {
            sw_timer_stop(&s_result_timer);
            enter_espera();
        }
        break;

    case ST_VALIDANDO:
    default:
        /* Inalcanzable: ST_VALIDANDO se resuelve mas abajo, dentro del mismo
         * paso en que se alcanza. */
        enter_espera();
        break;
    }

    /* CRITICO: ST_VALIDANDO es un estado DE PASO.
     *
     * Se atraviesa en el mismo paso en que se completa el cuarto digito y nunca
     * sobrevive al siguiente. Es el unico punto del programa que compara la
     * contrasena, asi que no existe forma de validar dos veces el mismo
     * ingreso. Misma tecnica que KEY_PRESSED en la MEF de antirrebote. */
    if (s_state == ST_VALIDANDO) {

        if (password_matches() != 0u) {
            s_failed_attempts = 0u;
            s_display.view    = SYS_VIEW_GRANTED;
            s_state           = ST_ACCESO;
        } else {
            if (s_failed_attempts < MAX_FAILED_ATTEMPTS) {
                s_failed_attempts++;
            }
            s_display.view = SYS_VIEW_DENIED;
            s_state        = ST_ERROR;
        }

        sw_timer_start(&s_result_timer, RESULT_DISPLAY_MS);
    }
}

SysDisplay_t system_fsm_get_display(void)
{
    return s_display;
}

/** @brief Vuelve a ESPERA: buffer vacio y vista de sistema listo. */
static void enter_espera(void)
{
    password_clear();

    s_state         = ST_ESPERA;
    s_display.view  = SYS_VIEW_IDLE;
    s_display.digit = 0u;
    s_display.count = 0u;
}

/**
 * @brief Procesa una pulsacion validada estando en ESPERA o INGRESANDO.
 *
 * Reparto de las 16 teclas, segun lo acordado:
 *   - Las diez numericas ocupan una posicion del buffer.
 *   - `*` borra el ingreso y devuelve el sistema a ESPERA.
 *   - `A`, `B`, `C`, `D` y `#` se ignoran por completo: ni cuentan como digito
 *     ni interrumpen un ingreso en curso.
 */
static void handle_pressed(uint8_t key)
{
    if (password_key_is_clear(key) != 0u) {
        enter_espera();
        return;
    }

    const uint8_t digit = password_key_to_digit(key);

    if (digit == PASSWORD_NO_DIGIT) {
        return;   /* Tecla no numerica distinta de '*': se ignora. */
    }

    if (password_add_digit(digit) == 0u) {
        return;   /* Buffer lleno; no deberia ocurrir, se ignora por seguridad. */
    }

    /* Mientras la tecla siga pulsada se muestra el digito. Al soltarla, el
     * evento RELEASED cambiara la vista a la barra de progreso. */
    s_display.view  = SYS_VIEW_DIGIT;
    s_display.digit = digit;
    s_display.count = password_count();

    if (password_is_complete() != 0u) {
        s_state = ST_VALIDANDO;
    } else {
        s_state = ST_INGRESANDO;
    }
}
