/**
 * @file    main.c
 * @brief   Fase 1 - Validacion de la cadena de compilacion y de la base de tiempo.
 *
 * Parpadeo del LED D2 de la placa a 1 Hz (500 ms encendido, 500 ms apagado)
 * usando SysTick y un temporizador software, sin ninguna espera bloqueante.
 *
 * Sirve para comprobar tres cosas de una vez:
 *   1. Que el proyecto compila y el .hex se flashea correctamente.
 *   2. Que la capa gpio escribe de verdad sobre los registros.
 *   3. Que la base de tiempo mide 1 ms real. Si el reloj no fuese el HSI a
 *      16 MHz, el parpadeo saldria proporcionalmente rapido o lento.
 *
 * La estructura del while(1) ya es la definitiva: en la Fase 6 solo habra que
 * sustituir el bloque del parpadeo por las llamadas a las cinco maquinas de
 * estados, en orden fijo.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include <stdint.h>

#include "board.h"
#include "gpio.h"
#include "timebase.h"

/** Semiperiodo del parpadeo. 500 ms encendido + 500 ms apagado = 1 Hz. */
#define BLINK_HALF_PERIOD_MS        500u

static void debug_led_init(void);
static void debug_led_write(uint8_t on);

int main(void)
{
    SwTimer_t blink_timer;
    uint8_t   led_on = 0u;

    /* --- Inicializacion --------------------------------------------------- */
    debug_led_init();
    timebase_init();

    debug_led_write(led_on);
    sw_timer_start(&blink_timer, BLINK_HALF_PERIOD_MS);

    /* --- Bucle principal cooperativo -------------------------------------- */
    while (1) {

        /* Todo el trabajo se hace al ritmo del tick de 1 ms. Entre tick y tick
         * el bucle simplemente da vueltas: nunca se queda esperando dentro de
         * ninguna funcion. */
        if (timebase_tick_ready() != 0u) {

            /* En la Fase 6 este bloque sera:
             *   keypad_scan_step();
             *   keypad_debounce_step();
             *   system_fsm_step(evento);
             *   led_matrix_mux_step();
             */
            if (sw_timer_expired(&blink_timer) != 0u) {
                led_on = (led_on == 0u) ? 1u : 0u;
                debug_led_write(led_on);

                /* El temporizador no se rearma solo: se decide aqui. */
                sw_timer_start(&blink_timer, BLINK_HALF_PERIOD_MS);
            }
        }
    }
}

/**
 * @brief Configura el pin del LED de diagnostico como salida push-pull.
 *
 * Nota de arquitectura: cada modulo configura sus propios pines. Aqui solo se
 * configura el LED porque es lo unico que usa esta fase; en las siguientes,
 * led_matrix_init() y keypad_init() haran lo propio con sus buses.
 */
static void debug_led_init(void)
{
    /* Primero el reloj del puerto, siempre. Sin el, las escrituras siguientes
     * se perderian sin dar ningun error. */
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
 * El LED D2 de la placa tiene el anodo a 3V3 y el catodo al pin, asi que es
 * ACTIVO EN BAJO: hay que escribir 0 para encenderlo. La inversion se resuelve
 * aqui, a partir del #define de board.h, para que el resto del programa pueda
 * razonar en terminos de "encendido / apagado" y no de niveles logicos.
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
