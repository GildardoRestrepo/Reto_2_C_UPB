/**
 * @file    timebase.c
 * @brief   Implementacion de la base de tiempo con SysTick.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include "timebase.h"
#include "board.h"
#include "stm32f4xx.h"

/** Frecuencia de la base de tiempo: un tick por milisegundo. */
#define TIMEBASE_TICK_HZ            1000u

/**
 * Contador de milisegundos.
 *
 * CRITICO: es 'volatile' porque lo escribe la interrupcion de SysTick y lo lee
 * el while(1). Sin 'volatile' el compilador ve un bucle que consulta una
 * variable que nadie modifica dentro del bucle, la cachea en un registro del
 * nucleo y el programa se queda esperando eternamente un cambio que si ocurrio
 * en memoria pero que ya no vuelve a leer.
 */
static volatile uint32_t s_millis = 0u;

/**
 * Ultimo milisegundo que el bucle principal alcanzo a atender.
 *
 * No lleva 'volatile': solo lo toca el while(1), nunca la interrupcion.
 */
static uint32_t s_last_serviced = 0u;

/**
 * @brief Rutina de interrupcion de SysTick. Se ejecuta cada 1 ms.
 *
 * El nombre no es casual: el archivo de arranque (startup_stm32f407vetx.s) ya
 * declara 'SysTick_Handler' como simbolo debil apuntando a un bucle infinito.
 * Al definirlo aqui, el enlazador sustituye aquel por este. No hay que tocar el
 * NVIC: SysTick es una excepcion del nucleo Cortex-M4, no una interrupcion de
 * periferico, y se habilita desde su propio registro CTRL.
 *
 * CRITICO: la rutina hace lo minimo imprescindible. Meter logica aqui seria
 * bloquear el sistema desde otro sitio, que es justo lo que el reto prohibe.
 * Todo el trabajo real ocurre en el while(1).
 */
void SysTick_Handler(void)
{
    s_millis++;
}

void timebase_init(void)
{
    s_millis        = 0u;
    s_last_serviced = 0u;

    /* Recarga del contador.
     *
     * SysTick cuenta hacia ABAJO y genera la excepcion en la transicion de 1 a
     * 0, por eso se resta uno: para un periodo de N ciclos hay que cargar N-1.
     *
     *   LOAD = (16 000 000 / 1000) - 1 = 15 999
     *
     * El campo RELOAD es de 24 bits (maximo 0xFFFFFF), lo que a 16 MHz da un
     * periodo maximo de 1,048 s. Esa es la razon de que los 3 s del enunciado
     * se cuenten acumulando ticks en software y no en el propio SysTick. */
    SysTick->LOAD = (BOARD_SYSCLK_HZ / TIMEBASE_TICK_HZ) - 1u;

    /* Escribir cualquier valor en VAL pone el contador a 0 y limpia COUNTFLAG,
     * de modo que el primer periodo sea completo y no uno truncado. */
    SysTick->VAL = 0u;

    /* CLKSOURCE = 1 -> reloj del procesador (16 MHz) y no el dividido por 8.
     * TICKINT   = 1 -> genera la excepcion al llegar a cero.
     * ENABLE    = 1 -> arranca el contador.
     * Este es el ultimo paso a proposito: no se arranca hasta que LOAD y VAL
     * ya tienen el valor correcto. */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk
                  | SysTick_CTRL_ENABLE_Msk;
}

uint32_t timebase_now_ms(void)
{
    /* Una lectura de 32 bits alineada es atomica en Cortex-M4, asi que no hace
     * falta deshabilitar interrupciones para leer el contador. */
    return s_millis;
}

uint8_t timebase_tick_ready(void)
{
    /* CRITICO: se compara contra el contador en lugar de usar una bandera.
     *
     * Con una bandera "hubo tick" habria una carrera: si SysTick interrumpe
     * justo entre leer la bandera y limpiarla, ese tick se perderia. Ademas, si
     * el bucle llegara tarde y se acumularan varios ticks, la bandera solo
     * podria representar uno.
     *
     * Comparando contra el contador e incrementando s_last_serviced de UNO EN
     * UNO, no se pierde ningun tick: si el bucle se retrasa, en las siguientes
     * vueltas se pone al dia. */
    if (s_millis != s_last_serviced) {
        s_last_serviced++;
        return 1u;
    }

    return 0u;
}

void sw_timer_start(SwTimer_t *timer, uint32_t period_ms)
{
    timer->start_ms  = timebase_now_ms();
    timer->period_ms = period_ms;
    timer->running   = 1u;
}

void sw_timer_stop(SwTimer_t *timer)
{
    timer->running = 0u;
}

uint8_t sw_timer_expired(const SwTimer_t *timer)
{
    if (timer->running == 0u) {
        return 0u;
    }

    /* CRITICO: la comparacion es (ahora - inicio) >= periodo, con aritmetica
     * SIN SIGNO. Escrito asi, el temporizador sigue siendo correcto cuando el
     * contador de milisegundos desborda a los 49,7 dias: la resta en uint32_t
     * da igualmente la distancia real entre los dos instantes.
     *
     * La forma ingenua (ahora >= inicio + periodo) SI falla al desbordar,
     * porque 'inicio + periodo' se envuelve y la comparacion se vuelve falsa
     * durante los 49,7 dias siguientes. */
    if ((timebase_now_ms() - timer->start_ms) >= timer->period_ms) {
        return 1u;
    }

    return 0u;
}
