/**
 * @file    led_matrix.c
 * @brief   Implementacion del driver de la matriz LED 8x8 (OUT-OUT).
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include "led_matrix.h"
#include "board.h"
#include "gpio.h"

/** Mascara de los 8 pines de fila dentro de su puerto (PE8..PE15 -> 0xFF00). */
#define ROW_MASK    ((uint32_t)0xFFu << MATRIX_ROW_FIRST_PIN)
/** Mascara de los 8 pines de columna dentro de su puerto (PD0..PD7 -> 0x00FF). */
#define COL_MASK    ((uint32_t)0xFFu << MATRIX_COL_FIRST_PIN)

/**
 * Framebuffer, ya traducido al ORDEN DE HARDWARE: el bit n de cada byte
 * corresponde al pin de columna n. La conversion desde el convenio de los
 * bitmaps se hace en led_matrix_show().
 */
static uint8_t s_framebuffer[MATRIX_ROWS];

/**
 * Estado de la MEF de multiplexado: la fila que esta encendida ahora mismo.
 * Avanza de 0 a 7 y vuelve a empezar, un paso por tick.
 */
static uint8_t s_active_row;

static uint8_t reverse_byte(uint8_t value);
static void    columns_off(void);
static void    columns_write(uint8_t hw_pattern);
static void    rows_all_off(void);
static void    row_select(uint8_t row);

void led_matrix_init(void)
{
    gpio_enable_port_clock(MATRIX_ROW_PORT);
    gpio_enable_port_clock(MATRIX_COL_PORT);

    /* CRITICO: se deja el nivel seguro ANTES de configurar los pines como
     * salida. El registro ODR se puede escribir aunque el pin siga en modo
     * entrada, asi que al conmutar MODER el pin ya sale con el valor correcto.
     * Al reves, entre el cambio de MODER y la primera escritura habria un
     * instante con toda la matriz encendida: un destello visible en cada
     * arranque. */
    rows_all_off();
    columns_off();

    gpio_config_group(MATRIX_ROW_PORT, MATRIX_ROW_FIRST_PIN, MATRIX_ROWS,
                      GPIO_MODE_OUTPUT, GPIO_OTYPE_PUSHPULL,
                      GPIO_PULL_NONE, GPIO_SPEED_LOW);

    gpio_config_group(MATRIX_COL_PORT, MATRIX_COL_FIRST_PIN, MATRIX_COLS,
                      GPIO_MODE_OUTPUT, GPIO_OTYPE_PUSHPULL,
                      GPIO_PULL_NONE, GPIO_SPEED_LOW);

    led_matrix_clear();
    s_active_row = 0u;
}

void led_matrix_clear(void)
{
    for (uint8_t r = 0u; r < MATRIX_ROWS; r++) {
        s_framebuffer[r] = 0u;
    }
}

void led_matrix_show(const uint8_t *frame)
{
    /* Unica traduccion entre el bitmap y el hardware: invertir el orden de los
     * bits de cada fila.
     *
     * En los bitmaps el bit 7 es la columna IZQUIERDA (ver images.h), porque asi
     * el arreglo escrito en binario se lee igual que se ve en la matriz. En el
     * hardware, en cambio, la columna izquierda es la primera del bus, o sea el
     * bit 0. De ahi la inversion.
     *
     * Se hace aqui, al cambiar de imagen, y no en el multiplexado, que se
     * ejecuta mil veces por segundo. */
    for (uint8_t r = 0u; r < MATRIX_ROWS; r++) {
        s_framebuffer[r] = reverse_byte(frame[r]);
    }
}

void led_matrix_mux_step(void)
{
    /* CRITICO: los tres pasos van SIEMPRE en este orden.
     *
     * Si se cambiara de fila antes de apagar las columnas, durante ese instante
     * la fila nueva se iluminaria con el patron de la anterior. Eso es el
     * ghosting: un fantasma tenue de la fila previa superpuesto a cada fila. */

    /* 1. Blanking. */
    columns_off();

    /* 2. Avanzar la MEF y seleccionar la nueva fila. */
    s_active_row = (uint8_t)((s_active_row + 1u) % MATRIX_ROWS);
    row_select(s_active_row);

    /* 3. Volcar el patron de esa fila. */
    columns_write(s_framebuffer[s_active_row]);
}

/**
 * @brief Invierte el orden de los bits de un byte (bit 7 <-> bit 0).
 *
 * Clasico intercambio por mitades, luego por pares y luego por bits: tres
 * operaciones en lugar de un bucle de ocho iteraciones.
 */
static uint8_t reverse_byte(uint8_t value)
{
    value = (uint8_t)(((value & 0xF0u) >> 4) | ((value & 0x0Fu) << 4));
    value = (uint8_t)(((value & 0xCCu) >> 2) | ((value & 0x33u) << 2));
    value = (uint8_t)(((value & 0xAAu) >> 1) | ((value & 0x55u) << 1));
    return value;
}

/** @brief Apaga las ocho columnas (blanking). */
static void columns_off(void)
{
#if (MATRIX_COL_ACTIVE_HIGH != 0)
    gpio_write_masked(MATRIX_COL_PORT, COL_MASK, 0u);
#else
    gpio_write_masked(MATRIX_COL_PORT, COL_MASK, COL_MASK);
#endif
}

/**
 * @brief Escribe el patron de columnas de la fila activa.
 * @param hw_pattern Patron ya en orden de hardware: bit n = columna n.
 */
static void columns_write(uint8_t hw_pattern)
{
    uint32_t value = ((uint32_t)hw_pattern << MATRIX_COL_FIRST_PIN);

#if (MATRIX_COL_ACTIVE_HIGH == 0)
    value = (~value) & COL_MASK;
#endif

    gpio_write_masked(MATRIX_COL_PORT, COL_MASK, value);
}

/** @brief Desactiva las ocho filas. */
static void rows_all_off(void)
{
#if (MATRIX_ROW_ACTIVE_LOW != 0)
    /* Filas activas en bajo: desactivadas significa todas a 1. */
    gpio_write_masked(MATRIX_ROW_PORT, ROW_MASK, ROW_MASK);
#else
    gpio_write_masked(MATRIX_ROW_PORT, ROW_MASK, 0u);
#endif
}

/**
 * @brief Activa una unica fila y desactiva las otras siete.
 *
 * Es una sola escritura a BSRR: no existe ningun instante con dos filas
 * activas a la vez.
 */
static void row_select(uint8_t row)
{
    const uint32_t row_bit = ((uint32_t)1u << (MATRIX_ROW_FIRST_PIN + row));

#if (MATRIX_ROW_ACTIVE_LOW != 0)
    gpio_write_masked(MATRIX_ROW_PORT, ROW_MASK, ROW_MASK & ~row_bit);
#else
    gpio_write_masked(MATRIX_ROW_PORT, ROW_MASK, row_bit);
#endif
}
