/**
 * @file    main.c
 * @brief   Fase 5 - Sistema de control de acceso completo.
 *
 * Aqui el reto funciona de punta a punta: se teclea una contrasena de cuatro
 * digitos, el sistema la valida y muestra el resultado durante 3 s antes de
 * volver a quedar listo.
 *
 * ESTRUCTURA DEL BUCLE. Cada tick de 1 ms se despachan, en orden fijo, las
 * cuatro maquinas de estados:
 *
 *      keypad_scan_step()    barrido del teclado      (hardware)
 *      keypad_fsm_step()     antirrebote              (evento)
 *      system_fsm_step()     logica de contrasena     (aplicacion)
 *      led_matrix_mux_step() multiplexado             (display)
 *
 * Es exactamente la cadena DRIVER -> EVENTO -> LOGICA -> DISPLAY del enunciado.
 * Ninguna de las cuatro espera a las demas y ninguna bloquea.
 *
 * ESTE ARCHIVO NO TIENE LOGICA DE PRODUCTO. Solo inicializa, despacha y traduce
 * la vista que pide system_fsm al mapa de bits concreto. La MEF de la
 * aplicacion no sabe que existe una matriz de LED; este archivo no sabe que
 * existe una contrasena.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include <stdint.h>

#include "board.h"
#include "images.h"
#include "keypad.h"
#include "keypad_fsm.h"
#include "led_matrix.h"
#include "password.h"
#include "system_fsm.h"
#include "timebase.h"

/** Filas centrales donde se dibuja la barra de progreso del ingreso. */
#define PROGRESS_ROW_TOP        3u
#define PROGRESS_ROW_BOTTOM     4u

/** Ancho en LED de cada bloque de la barra (8 columnas / 4 digitos). */
#define PROGRESS_BLOCK_WIDTH    (MATRIX_COLS / PASSWORD_LENGTH)

static void    render(const SysDisplay_t *display, uint8_t *frame);
static void    build_progress_frame(uint8_t count, uint8_t *frame);
static uint8_t display_changed(const SysDisplay_t *a, const SysDisplay_t *b);

int main(void)
{
    uint8_t      frame[MATRIX_ROWS];
    SysDisplay_t previous;

    /* --- Inicializacion --------------------------------------------------- */
    led_matrix_init();
    keypad_init();
    keypad_fsm_init();
    system_fsm_init();
    timebase_init();

    /* Valores imposibles: fuerzan el primer pintado en la primera vuelta. */
    previous.view  = SYS_VIEW_LOCKED;
    previous.digit = 0xFFu;
    previous.count = 0xFFu;

    /* --- Bucle principal cooperativo -------------------------------------- */
    while (1) {

        if (timebase_tick_ready() != 0u) {

            /* 1. Barrido del teclado. */
            keypad_scan_step();

            /* 2. Antirrebote: convierte el barrido en eventos. */
            keypad_fsm_step();

            /* 3. Logica de la aplicacion. Se llama SIEMPRE, aunque no haya
             *    evento: la maquina necesita el tick para vigilar el
             *    temporizador de los 3 segundos. */
            system_fsm_step(keypad_fsm_get_event());

            /* 4. Traducir la vista pedida a un mapa de bits, solo si cambio. */
            const SysDisplay_t current = system_fsm_get_display();

            if (display_changed(&current, &previous) != 0u) {
                render(&current, frame);
                previous = current;
            }

            /* 5. Multiplexado de la matriz. */
            led_matrix_mux_step();
        }
    }
}

/**
 * @brief Traduce una vista de la aplicacion al mapa de bits que la representa.
 *
 * Este es el unico punto del programa donde se decide QUE DIBUJO corresponde a
 * cada estado. Cambiar el aspecto del sistema es tocar solo esta funcion y los
 * bitmaps de images.c.
 */
static void render(const SysDisplay_t *display, uint8_t *frame)
{
    switch (display->view) {

    case SYS_VIEW_DIGIT:
        /* La comprobacion sobra por construccion, pero indexar un arreglo con
         * un valor recibido sin verificar es justo el tipo de descuido que
         * convierte un fallo logico en memoria corrupta. */
        if (display->digit < 10u) {
            led_matrix_show(img_digit[display->digit]);
        } else {
            led_matrix_show(img_idle_corners);
        }
        break;

    case SYS_VIEW_ENTERING:
        build_progress_frame(display->count, frame);
        led_matrix_show(frame);
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
 * @param frame Destino, MATRIX_ROWS bytes.
 */
static void build_progress_frame(uint8_t count, uint8_t *frame)
{
    for (uint8_t r = 0u; r < MATRIX_ROWS; r++) {
        frame[r] = 0u;
    }

    if (count == 0u) {
        return;
    }

    const uint8_t blocks  = (count > PASSWORD_LENGTH) ? (uint8_t)PASSWORD_LENGTH : count;
    const uint8_t lit     = (uint8_t)(blocks * PROGRESS_BLOCK_WIDTH);
    const uint8_t pattern = (uint8_t)(0xFFu << (MATRIX_COLS - lit));

    frame[PROGRESS_ROW_TOP]    = pattern;
    frame[PROGRESS_ROW_BOTTOM] = pattern;
}

/**
 * @brief 1 si las dos peticiones de pantalla difieren.
 *
 * Evita reconstruir el framebuffer en cada tick: la imagen solo cambia cuando
 * cambia el estado de la aplicacion, que es del orden de una vez por pulsacion.
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
