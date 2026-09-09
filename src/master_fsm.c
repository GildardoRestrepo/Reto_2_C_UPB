/**
 * @file    master_fsm.c
 * @brief   Implementacion de la MEF maestra.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include "master_fsm.h"

#include <stdint.h>

#include "board.h"
#include "images.h"
#include "keypad.h"
#include "keypad_fsm.h"
#include "led_matrix.h"
#include "password.h"
#include "system_fsm.h"
#include "timebase.h"

/** Duracion del autotest de arranque. */
#define SELFTEST_DURATION_MS    1000u

/** Filas centrales donde se dibuja la barra de progreso del ingreso. */
#define PROGRESS_ROW_TOP        3u
#define PROGRESS_ROW_BOTTOM     4u

/** Ancho en LED de cada bloque de la barra (8 columnas / 4 digitos). */
#define PROGRESS_BLOCK_WIDTH    (MATRIX_COLS / PASSWORD_LENGTH)

/** Estados. Ver el diagrama en _docs/architecture.md, seccion 5. */
typedef enum {
    MASTER_INIT = 0u,   /**< Configurando relojes, GPIO y modulos.     */
    MASTER_SELFTEST,    /**< Autotest de arranque en la matriz.        */
    MASTER_RUN          /**< Despacho cooperativo, regimen permanente. */
} MasterState_t;

static MasterState_t s_state;

/** Temporizador del autotest de arranque. */
static SwTimer_t s_selftest_timer;

/** Ultima vista dibujada, para no repintar en cada tick. */
static SysDisplay_t s_previous;

/** Lienzo de las imagenes que se generan por codigo (la barra de progreso). */
static uint8_t s_frame[MATRIX_ROWS];

static void    run_step(void);
static void    render(const SysDisplay_t *display);
static void    build_progress_frame(uint8_t count);
static uint8_t display_changed(const SysDisplay_t *a, const SysDisplay_t *b);
static void    force_redraw(void);

void master_fsm_init(void)
{
    s_state = MASTER_INIT;

    /* Orden de inicializacion. Cada modulo configura sus propios pines; nadie
     * toca los de otro.
     *
     * timebase va EL ULTIMO a proposito: arranca SysTick, y no interesa que la
     * interrupcion empiece a contar antes de que el resto este configurado. */
    led_matrix_init();
    keypad_init();
    keypad_fsm_init();
    system_fsm_init();
    timebase_init();

    /* Transicion a MASTER_SELFTEST.
     *
     * El autotest no es decorativo: enciende los 64 LED durante un segundo, de
     * modo que en cada arranque se ve si algun LED o algun hilo dejo de
     * responder. Es la misma imagen que localizo los fallos de cableado en la
     * Fase 2. */
    led_matrix_show(img_test_all);
    sw_timer_start(&s_selftest_timer, SELFTEST_DURATION_MS);

    s_state = MASTER_SELFTEST;
}

void master_fsm_step(void)
{
    switch (s_state) {

    case MASTER_SELFTEST:
        if (sw_timer_expired(&s_selftest_timer) != 0u) {
            sw_timer_stop(&s_selftest_timer);
            force_redraw();
            s_state = MASTER_RUN;
        }
        break;

    case MASTER_RUN:
        run_step();
        break;

    case MASTER_INIT:
    default:
        /* Inalcanzable: master_fsm_init() no deja la maquina en este estado. */
        break;
    }

    /* CRITICO: el multiplexado se ejecuta SIEMPRE, sea cual sea el estado.
     *
     * Tambien durante el autotest, y tambien durante los 3 segundos de la
     * imagen de resultado. Es lo que permite afirmar que ningun estado del
     * sistema congela el display, que es justo lo que el enunciado prohibe. */
    led_matrix_mux_step();
}

/**
 * @brief Regimen permanente: despacha las tres MEF de entrada y actualiza el
 *        display.
 *
 * El ORDEN es deliberado y sigue la cadena que pide el enunciado:
 *
 *      DRIVER  ->  EVENTO  ->  LOGICA  ->  DISPLAY
 *
 * Cada eslabon consume lo que produjo el anterior en este mismo tick, de modo
 * que una pulsacion recorre toda la cadena sin esperar a la vuelta siguiente.
 */
static void run_step(void)
{
    /* 1. DRIVER: explora el teclado. */
    keypad_scan_step();

    /* 2. EVENTO: filtra rebotes y publica pulsaciones validadas. */
    keypad_fsm_step();

    /* 3. LOGICA: se llama siempre, tambien sin evento, porque la maquina
     *    necesita el tick para vigilar el temporizador de los 3 segundos. */
    system_fsm_step(keypad_fsm_get_event());

    /* 4. DISPLAY: traducir la vista pedida, solo si cambio. */
    const SysDisplay_t current = system_fsm_get_display();

    if (display_changed(&current, &s_previous) != 0u) {
        render(&current);
        s_previous = current;
    }
}

/**
 * @brief Traduce una vista de la aplicacion al mapa de bits que la representa.
 *
 * Unico punto del programa donde se decide QUE DIBUJO corresponde a cada
 * estado. Cambiar el aspecto del sistema es tocar solo esta funcion y los
 * bitmaps de images.c.
 */
static void render(const SysDisplay_t *display)
{
    switch (display->view) {

    case SYS_VIEW_DIGIT:
        /* La comprobacion sobra por construccion, pero indexar un arreglo con
         * un valor recibido sin verificar es justo el tipo de descuido que
         * convierte un fallo logico en memoria corrupta. */
        if (display->digit < IMG_DIGIT_COUNT) {
            led_matrix_show(img_digit[display->digit]);
        } else {
            led_matrix_show(img_idle_corners);
        }
        break;

    case SYS_VIEW_ENTERING:
        build_progress_frame(display->count);
        led_matrix_show(s_frame);
        break;

    case SYS_VIEW_GRANTED:
        led_matrix_show(img_check);
        break;

    case SYS_VIEW_DENIED:
        led_matrix_show(img_cross);
        break;

    case SYS_VIEW_LOCKED:
        /* Reservado para la Fase 7. Hasta entonces es inalcanzable. */
    case SYS_VIEW_IDLE:
    default:
        led_matrix_show(img_idle_corners);
        break;
    }
}

/**
 * @brief Barra de progreso del ingreso, en las dos filas centrales.
 *
 * Un bloque de dos columnas por digito, creciendo de izquierda a derecha. Con
 * cuatro digitos la barra ocupa exactamente el ancho de la matriz, asi que de
 * un vistazo se sabe cuanto falta.
 *
 * @param count Digitos ingresados, 0..PASSWORD_LENGTH.
 */
static void build_progress_frame(uint8_t count)
{
    for (uint8_t r = 0u; r < MATRIX_ROWS; r++) {
        s_frame[r] = 0u;
    }

    if (count == 0u) {
        return;
    }

    const uint8_t blocks  = (count > PASSWORD_LENGTH) ? (uint8_t)PASSWORD_LENGTH : count;
    const uint8_t lit     = (uint8_t)(blocks * PROGRESS_BLOCK_WIDTH);
    const uint8_t pattern = (uint8_t)(0xFFu << (MATRIX_COLS - lit));

    s_frame[PROGRESS_ROW_TOP]    = pattern;
    s_frame[PROGRESS_ROW_BOTTOM] = pattern;
}

/**
 * @brief 1 si las dos peticiones de pantalla difieren.
 *
 * Evita reconstruir el framebuffer en cada tick: la imagen solo cambia cuando
 * cambia el estado de la aplicacion, del orden de una vez por pulsacion.
 */
static uint8_t display_changed(const SysDisplay_t *a, const SysDisplay_t *b)
{
    if (a->view != b->view) {
        return 1u;
    }

    if (a->digit != b->digit) {
        return 1u;
    }

    if (a->count != b->count) {
        return 1u;
    }

    return 0u;
}

/**
 * @brief Invalida la vista recordada para forzar un repintado.
 *
 * Se usa al salir del autotest: la matriz muestra el patron de prueba, que no
 * corresponde a ninguna vista de la aplicacion, asi que hay que obligar al
 * primer paso de MASTER_RUN a dibujar aunque la vista no haya cambiado.
 */
static void force_redraw(void)
{
    s_previous.view  = SYS_VIEW_LOCKED;
    s_previous.digit = 0xFFu;
    s_previous.count = 0xFFu;
}
