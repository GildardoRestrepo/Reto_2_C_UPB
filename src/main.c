/**
 * @file    main.c
 * @brief   Punto de entrada del sistema de control de acceso.
 *
 * Todo el programa cabe en una idea: **una base de tiempo de 1 ms y una
 * maquina de estados maestra que se ejecuta una vez por tick**.
 *
 * Este archivo no contiene logica de ninguna clase. La coordinacion vive en
 * `master_fsm`, la logica del producto en `system_fsm`, y el acceso al
 * hardware en los drivers. Aqui solo queda el latido.
 *
 * Que ningun estado del sistema bloquee es visible desde aqui: el bucle nunca
 * espera dentro de una funcion, solo comprueba si toca el siguiente
 * milisegundo.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#include "master_fsm.h"
#include "timebase.h"

int main(void)
{
    /* Configura relojes, GPIO y los cinco modulos, y arranca SysTick. */
    master_fsm_init();

    while (1) {

        /* Un paso de la maquina maestra por cada milisegundo transcurrido.
         *
         * timebase_tick_ready() consume los ticks de uno en uno, de modo que si
         * una vuelta llegara tarde el sistema se pondria al dia en las
         * siguientes en lugar de perder el tiempo atrasado. */
        if (timebase_tick_ready() != 0u) {
            master_fsm_step();
        }
    }
}
