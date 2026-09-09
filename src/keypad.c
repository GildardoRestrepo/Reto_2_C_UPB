/**
 * @file    keypad.c
 * @brief   Implementacion del driver del teclado 4x4 (IN-OUT).
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include "keypad.h"
#include "board.h"
#include "gpio.h"

/** Mascara de los 4 pines de fila dentro de su puerto (PB6..PB9 -> 0x03C0). */
#define ROW_MASK    ((uint32_t)0x0Fu << KEYPAD_ROW_FIRST_PIN)
/** Mascara de los 4 pines de columna dentro de su puerto (PA1..PA4 -> 0x001E). */
#define COL_MASK    ((uint32_t)0x0Fu << KEYPAD_COL_FIRST_PIN)

/**
 * Estados de la MEF de barrido.
 *
 * Cada fila necesita dos ticks. El tick que separa DRIVE de READ ES el tiempo
 * de asentamiento de las lineas: es lo que sustituye a la espera activa que
 * usaria una implementacion bloqueante. Con el pull-up interno de unos 40 kOhm
 * y la capacidad del cableado, 1 ms sobra con mucho margen.
 */
typedef enum {
    SCAN_DRIVE_ROW = 0u,    /**< Activa la fila r y cede el control.      */
    SCAN_READ_COLS = 1u     /**< Lee las columnas y avanza a la siguiente. */
} ScanState_t;

static ScanState_t s_state;

/** Fila que se esta explorando, 0..3. */
static uint8_t s_scan_row;

/** Tecla encontrada en el barrido EN CURSO, todavia sin publicar. */
static uint8_t s_partial_key;

/** Ultima instantanea completa, la que ve el exterior. */
static uint8_t s_raw_key;

/** 1 mientras la instantanea actual no haya sido consumida. */
static uint8_t s_snapshot_ready;

static void    rows_all_inactive(void);
static void    row_activate(uint8_t row);
static uint8_t read_columns(void);
static uint8_t first_pressed_column(uint8_t columns);

void keypad_init(void)
{
    gpio_enable_port_clock(KEYPAD_ROW_PORT);
    gpio_enable_port_clock(KEYPAD_COL_PORT);

    /* Nivel seguro antes de conmutar MODER: ODR se puede escribir con el pin
     * todavia en modo entrada, asi que las filas nacen ya inactivas y no hay
     * ningun instante con las cuatro tirando a masa. */
    rows_all_inactive();

    /* Filas: salida OPEN-DRAIN. Escribir 1 no sube el pin, lo deja en alta
     * impedancia. Gracias a eso, pulsar dos teclas a la vez no puede conectar
     * una salida en alto contra otra en bajo. */
    gpio_config_group(KEYPAD_ROW_PORT, KEYPAD_ROW_FIRST_PIN, KEYPAD_ROWS,
                      GPIO_MODE_OUTPUT, GPIO_OTYPE_OPENDRAIN,
                      GPIO_PULL_NONE, GPIO_SPEED_LOW);

    /* Columnas: entrada con PULL-UP interno. Sin el, una columna sin tecla
     * pulsada quedaria flotando y leeria basura. */
    gpio_config_group(KEYPAD_COL_PORT, KEYPAD_COL_FIRST_PIN, KEYPAD_COLS,
                      GPIO_MODE_INPUT, GPIO_OTYPE_PUSHPULL,
                      GPIO_PULL_UP, GPIO_SPEED_LOW);

    s_state          = SCAN_DRIVE_ROW;
    s_scan_row       = 0u;
    s_partial_key    = KEYPAD_NO_KEY;
    s_raw_key        = KEYPAD_NO_KEY;
    s_snapshot_ready = 0u;
}

void keypad_scan_step(void)
{
    if (s_state == SCAN_DRIVE_ROW) {

        row_activate(s_scan_row);
        s_state = SCAN_READ_COLS;

    } else {

        const uint8_t columns = read_columns();

        /* Se queda con la primera tecla del barrido. Si hay varias pulsadas,
         * gana la de menor indice; el reto no pide multitecla. */
        if ((columns != 0u) && (s_partial_key == KEYPAD_NO_KEY)) {
            s_partial_key = (uint8_t)((s_scan_row * KEYPAD_COLS)
                                      + first_pressed_column(columns));
        }

        s_scan_row++;

        if (s_scan_row >= KEYPAD_ROWS) {
            /* CRITICO: la instantanea se publica solo al terminar las cuatro
             * filas, nunca a mitad de barrido. Asi el exterior siempre lee un
             * estado coherente del teclado completo y no una mezcla de dos
             * momentos distintos. */
            s_scan_row       = 0u;
            s_raw_key        = s_partial_key;
            s_partial_key    = KEYPAD_NO_KEY;
            s_snapshot_ready = 1u;
        }

        s_state = SCAN_DRIVE_ROW;
    }
}

uint8_t keypad_snapshot_ready(void)
{
    if (s_snapshot_ready != 0u) {
        s_snapshot_ready = 0u;
        return 1u;
    }

    return 0u;
}

uint8_t keypad_get_raw_key(void)
{
    return s_raw_key;
}

/** @brief Deja las cuatro filas inactivas (en alta impedancia). */
static void rows_all_inactive(void)
{
#if (KEYPAD_ROW_ACTIVE_LOW != 0)
    gpio_write_masked(KEYPAD_ROW_PORT, ROW_MASK, ROW_MASK);
#else
    gpio_write_masked(KEYPAD_ROW_PORT, ROW_MASK, 0u);
#endif
}

/**
 * @brief Activa una unica fila y deja las otras tres inactivas.
 *
 * Una sola escritura a BSRR: no existe ningun instante con dos filas activas,
 * que produciria lecturas ambiguas en las columnas.
 */
static void row_activate(uint8_t row)
{
    const uint32_t row_bit = ((uint32_t)1u << (KEYPAD_ROW_FIRST_PIN + row));

#if (KEYPAD_ROW_ACTIVE_LOW != 0)
    gpio_write_masked(KEYPAD_ROW_PORT, ROW_MASK, ROW_MASK & ~row_bit);
#else
    gpio_write_masked(KEYPAD_ROW_PORT, ROW_MASK, row_bit);
#endif
}

/**
 * @brief Lee las cuatro columnas.
 * @return Nibble donde el bit c a 1 significa COLUMNA c PULSADA.
 *
 * Se lee de IDR, que refleja el nivel fisico del pin. Leer de ODR devolveria
 * lo ultimo que escribimos, que no tiene nada que ver con la tecla.
 */
static uint8_t read_columns(void)
{
    const uint32_t idr = gpio_read_masked(KEYPAD_COL_PORT, COL_MASK);
    uint32_t       raw = (idr >> KEYPAD_COL_FIRST_PIN) & 0x0Fu;

#if (KEYPAD_COL_ACTIVE_LOW != 0)
    /* Con pull-up, en reposo se lee 1 y pulsada 0. Se invierte aqui para que
     * el resto del modulo razone con "1 = pulsada". */
    raw = (~raw) & 0x0Fu;
#endif

    return (uint8_t)raw;
}

/**
 * @brief Indice de la columna pulsada de menor peso.
 *
 * El bucle esta acotado a KEYPAD_COLS a proposito: aunque el llamante ya
 * garantiza que hay algun bit a 1, un bucle sin cota seria un cuelgue en
 * potencia dentro de un sistema que presume de no bloquear nunca.
 */
static uint8_t first_pressed_column(uint8_t columns)
{
    for (uint8_t c = 0u; c < KEYPAD_COLS; c++) {
        if ((columns & (uint8_t)(1u << c)) != 0u) {
            return c;
        }
    }

    return 0u;
}
