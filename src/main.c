/**
 * @file    main.c
 * @brief   Fase 2 - Driver de la matriz LED y MEF de multiplexado.
 *
 * Rota tres imagenes de diagnostico cada 2 s mientras multiplexa la matriz a
 * 125 Hz, y mantiene el parpadeo del LED D2 como latido del sistema.
 *
 * Que se valida aqui:
 *   1. Que el multiplexado OUT-OUT construye una imagen estable, sin parpadeo
 *      perceptible y sin ghosting.
 *   2. La ORIENTACION real del cableado. Las tres imagenes son asimetricas a
 *      proposito; segun como se vean, se ajustan MATRIX_ROW_REVERSE y
 *      MATRIX_COL_REVERSE en board.h y no se toca ningun bitmap.
 *   3. Que el sistema sigue siendo no bloqueante: el cambio de imagen cada 2 s
 *      se hace con un temporizador software mientras la matriz se refresca sin
 *      interrupcion.
 *
 * El LED D2 sigue parpadeando como latido: si la matriz no muestra nada pero
 * D2 parpadea, el fallo esta en el cableado o en el driver, no en el arranque
 * ni en la base de tiempo.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include <stdint.h>

#include "board.h"
#include "gpio.h"
#include "images.h"
#include "led_matrix.h"
#include "timebase.h"

/** Semiperiodo del latido. 500 ms encendido + 500 ms apagado = 1 Hz. */
#define BLINK_HALF_PERIOD_MS        500u

/** Tiempo que permanece visible cada imagen de diagnostico. */
#define DIAG_IMAGE_PERIOD_MS        2000u

/** Secuencia de diagnostico de la Fase 2, en orden de utilidad. */
static const uint8_t *const k_diag_images[] = {
    img_test_row0,      /* ¿estan cruzadas filas y columnas?      */
    img_test_f,         /* ¿esta espejada o girada?               */
    img_test_border,    /* ¿el marco cierra por los cuatro lados? */
    img_test_all        /* ¿hay alguna fila o columna muerta?     */
};

#define DIAG_IMAGE_COUNT    (sizeof(k_diag_images) / sizeof(k_diag_images[0]))

static void debug_led_init(void);
static void debug_led_write(uint8_t on);

int main(void)
{
    SwTimer_t blink_timer;
    SwTimer_t diag_timer;
    uint8_t   led_on      = 0u;
    uint8_t   image_index = 0u;

    /* --- Inicializacion --------------------------------------------------- */
    debug_led_init();
    led_matrix_init();
    timebase_init();

    debug_led_write(led_on);
    led_matrix_show(k_diag_images[image_index]);

    sw_timer_start(&blink_timer, BLINK_HALF_PERIOD_MS);
    sw_timer_start(&diag_timer, DIAG_IMAGE_PERIOD_MS);

    /* --- Bucle principal cooperativo -------------------------------------- */
    while (1) {

        if (timebase_tick_ready() != 0u) {

            /* Latido del sistema. */
            if (sw_timer_expired(&blink_timer) != 0u) {
                led_on = (led_on == 0u) ? 1u : 0u;
                debug_led_write(led_on);
                sw_timer_start(&blink_timer, BLINK_HALF_PERIOD_MS);
            }

            /* Rotacion de las imagenes de diagnostico. En la Fase 5 este bloque
             * lo sustituye la MEF de contrasena, que decidira que mostrar. */
            if (sw_timer_expired(&diag_timer) != 0u) {
                image_index++;
                if (image_index >= DIAG_IMAGE_COUNT) {
                    image_index = 0u;
                }
                led_matrix_show(k_diag_images[image_index]);
                sw_timer_start(&diag_timer, DIAG_IMAGE_PERIOD_MS);
            }

            /* El multiplexado va al final del tick, como en el orden de
             * despacho documentado en _docs/architecture.md. */
            led_matrix_mux_step();
        }
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
