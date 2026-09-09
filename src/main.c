/**
 * @file    main.c
 * @brief   Fase 4 - MEF de antirrebote y contrato de eventos.
 *
 * El bucle ya no lee la tecla cruda: consume EVENTOS. La matriz muestra el
 * bloque 2x2 de la tecla mantenida y vuelve a la imagen de reposo al soltarla,
 * y toda esa logica se sostiene unicamente sobre los eventos que emite la MEF
 * de antirrebote. main no consulta en ningun momento el estado interno del
 * teclado.
 *
 * PRUEBA DEL REQUISITO: D2 conmuta UNA VEZ por cada pulsacion validada.
 * Mantener una tecla pulsada cinco segundos cambia el LED una sola vez y ahi se
 * queda; pulsar diez veces produce diez cambios. Es la demostracion visual de
 * que cada pulsacion genera exactamente un evento.
 *
 * D2 deja de ser el latido del sistema: esa funcion la cumple ya la imagen de
 * esquinas de la matriz, que solo se ve si el multiplexado sigue corriendo.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include <stdint.h>

#include "board.h"
#include "gpio.h"
#include "images.h"
#include "keypad.h"
#include "keypad_fsm.h"
#include "led_matrix.h"
#include "timebase.h"

/** Lado en LED del bloque que representa una tecla (8 / 4 = 2). */
#define KEY_BLOCK_SIZE              (MATRIX_ROWS / KEYPAD_ROWS)

static void debug_led_init(void);
static void debug_led_write(uint8_t on);
static void build_key_frame(uint8_t key, uint8_t *frame);

int main(void)
{
    uint8_t frame[MATRIX_ROWS];
    uint8_t press_led = 0u;
    uint8_t held_key  = KEYPAD_NO_KEY;
    uint8_t redraw    = 1u;

    /* --- Inicializacion --------------------------------------------------- */
    debug_led_init();
    led_matrix_init();
    keypad_init();
    keypad_fsm_init();
    timebase_init();

    debug_led_write(press_led);

    /* --- Bucle principal cooperativo -------------------------------------- */
    while (1) {

        if (timebase_tick_ready() != 0u) {

            /* 1. MEF de barrido: explora el teclado. */
            keypad_scan_step();

            /* 2. MEF de antirrebote: convierte el flujo crudo en eventos. */
            keypad_fsm_step();

            /* 3. Consumir el evento. Este es el unico canal por el que main se
             *    entera de lo que pasa en el teclado: no hay ninguna consulta
             *    al estado interno de las maquinas de abajo. */
            const KeyEvent_t event = keypad_fsm_get_event();

            if (event.type == KEY_EVT_PRESSED) {
                /* La prueba del requisito: una conmutacion por pulsacion. */
                press_led = (press_led == 0u) ? 1u : 0u;
                debug_led_write(press_led);

                held_key = event.key;
                redraw   = 1u;

            } else if (event.type == KEY_EVT_RELEASED) {
                held_key = KEYPAD_NO_KEY;
                redraw   = 1u;

            } else {
                /* KEY_EVT_NONE: nada que hacer en este tick. */
            }

            /* 4. Repintar solo cuando algo cambio. */
            if (redraw != 0u) {
                if (held_key == KEYPAD_NO_KEY) {
                    led_matrix_show(img_idle_corners);
                } else {
                    build_key_frame(held_key, frame);
                    led_matrix_show(frame);
                }
                redraw = 0u;
            }

            /* 5. Multiplexado, al final del tick. */
            led_matrix_mux_step();
        }
    }
}

/**
 * @brief Construye la imagen del bloque 2x2 que representa una tecla.
 *
 * La tecla de indice k ocupa la fila k/4 y la columna k%4 del teclado, y se
 * dibuja como un cuadrado de 2x2 LED en esa misma posicion de la rejilla.
 *
 * El desplazamiento (6 - 2*columna) sale del convenio de los bitmaps: el bit 7
 * es la columna izquierda, asi que las dos columnas del bloque de la columna 0
 * son los bits 7 y 6, o sea 0b11 desplazado 6 posiciones.
 *
 * @param key   Indice de tecla 0..15, o KEYPAD_NO_KEY para dejarla en blanco.
 * @param frame Destino, MATRIX_ROWS bytes.
 */
static void build_key_frame(uint8_t key, uint8_t *frame)
{
    for (uint8_t r = 0u; r < MATRIX_ROWS; r++) {
        frame[r] = 0u;
    }

    if (key == KEYPAD_NO_KEY) {
        return;
    }

    const uint8_t key_row = (uint8_t)(key / KEYPAD_COLS);
    const uint8_t key_col = (uint8_t)(key % KEYPAD_COLS);
    const uint8_t pattern = (uint8_t)(0x03u << (6u - (2u * key_col)));

    for (uint8_t i = 0u; i < KEY_BLOCK_SIZE; i++) {
        frame[(KEY_BLOCK_SIZE * key_row) + i] = pattern;
    }
}

/**
 * @brief Configura el pin del LED de diagnostico como salida push-pull.
 */
static void debug_led_init(void)
{
    gpio_enable_port_clock(DEBUG_LED_PORT);

    gpio_config_pin(DEBUG_LED_PORT,
                    DEBUG_LED_PIN,
                    GPIO_MODE_OUTPUT,
                    GPIO_OTYPE_PUSHPULL,
                    GPIO_PULL_NONE,
                    GPIO_SPEED_LOW);
}

/**
 * @brief Enciende o apaga el LED de diagnostico.
 *
 * D2 tiene el anodo a 3V3 y el catodo al pin, asi que es ACTIVO EN BAJO. La
 * inversion se resuelve aqui para que el resto del programa razone en terminos
 * de encendido y apagado, no de niveles logicos.
 *
 * @param on 1 para encender, 0 para apagar.
 */
static void debug_led_write(uint8_t on)
{
    const uint32_t mask = (1u << DEBUG_LED_PIN);
    uint32_t       level;

#if (DEBUG_LED_ACTIVE_LOW != 0)
    level = (on != 0u) ? 0u : mask;
#else
    level = (on != 0u) ? mask : 0u;
#endif

    gpio_write_masked(DEBUG_LED_PORT, mask, level);
}
