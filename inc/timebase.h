/**
 * @file    timebase.h
 * @brief   Base de tiempo del sistema: SysTick a 1 ms y temporizadores software.
 *
 * Es el modulo del que cuelgan TODOS los tiempos del proyecto: el barrido del
 * teclado, el antirrebote, el multiplexado de la matriz y los 3 s de las
 * imagenes de resultado.
 *
 * En el proyecto no existe ninguna funcion de espera. Un tiempo largo no se
 * "gasta" bloqueando: se anota una marca de tiempo y se sigue ejecutando.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>

/**
 * @brief Configura SysTick para interrumpir cada 1 ms y lo arranca.
 *
 * Debe llamarse una sola vez, antes de entrar al while(1).
 */
void timebase_init(void);

/**
 * @brief Milisegundos transcurridos desde timebase_init().
 *
 * Desborda a los 2^32 ms (unos 49,7 dias). Los temporizadores de este modulo
 * siguen funcionando correctamente despues del desbordamiento; ver
 * sw_timer_expired().
 */
uint32_t timebase_now_ms(void);

/**
 * @brief Indica si hay un tick de 1 ms pendiente de atender, y lo consume.
 *
 * Es el latido del bucle principal: el while(1) llama a esta funcion y, cuando
 * devuelve 1, ejecuta un paso de cada maquina de estados.
 *
 * @return 1 si habia un tick pendiente (y queda consumido), 0 si no.
 */
uint8_t timebase_tick_ready(void);

/**
 * @brief Temporizador software no bloqueante.
 *
 * No cuenta por su cuenta: solo guarda cuando arranco y cuanto debe durar. La
 * comprobacion es una resta contra el contador de milisegundos, asi que tener
 * muchos temporizadores activos no cuesta absolutamente nada.
 */
typedef struct {
    uint32_t start_ms;   /**< Instante en que se arranco.             */
    uint32_t period_ms;  /**< Duracion pedida.                        */
    uint8_t  running;    /**< 1 si esta armado, 0 si esta detenido.   */
} SwTimer_t;

/** @brief Arranca (o rearma) el temporizador con el periodo indicado. */
void sw_timer_start(SwTimer_t *timer, uint32_t period_ms);

/** @brief Detiene el temporizador. sw_timer_expired() devolvera 0. */
void sw_timer_stop(SwTimer_t *timer);

/**
 * @brief Indica si el temporizador ya cumplio su periodo.
 *
 * No se rearma solo: el llamante decide si vuelve a arrancarlo o lo deja.
 *
 * @return 1 si vencio, 0 si sigue corriendo o esta detenido.
 */
uint8_t sw_timer_expired(const SwTimer_t *timer);

#endif /* TIMEBASE_H */
