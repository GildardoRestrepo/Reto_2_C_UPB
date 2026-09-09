/**
 * @file    master_fsm.h
 * @brief   MEF maestra: capa de integracion del sistema.
 *
 * Es la maquina que coordina a las otras cuatro. No contiene logica de
 * producto: inicializa los modulos, ordena el despacho y traduce la vista que
 * pide la aplicacion al mapa de bits concreto que entiende el display.
 *
 * Reparto de responsabilidades:
 *   - `system_fsm` decide QUE mostrar, sin saber que existe una matriz de LED.
 *   - `master_fsm` decide COMO se dibuja, sin saber que existe una contrasena.
 *   - `main.c` solo arranca la maquina y le da el latido de 1 ms.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#ifndef MASTER_FSM_H
#define MASTER_FSM_H

/**
 * @brief Inicializa todos los modulos y deja la maquina lista para arrancar.
 *
 * Cubre el estado MASTER_INIT completo y transiciona a MASTER_SELFTEST. Al
 * volver de esta funcion la base de tiempo ya esta corriendo.
 */
void master_fsm_init(void);

/**
 * @brief Ejecuta un paso de la maquina maestra.
 *
 * Debe llamarse exactamente una vez por tick de 1 ms desde el while(1).
 */
void master_fsm_step(void);

#endif /* MASTER_FSM_H */
