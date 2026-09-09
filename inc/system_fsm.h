/**
 * @file    system_fsm.h
 * @brief   MEF de la aplicacion: ingreso y validacion de la contrasena.
 *
 * Es la logica del producto. NO conoce hardware: recibe eventos de teclado y
 * publica que VISTA debe mostrarse. Quien traduce esa vista a un mapa de bits
 * concreto es la capa de integracion, no esta maquina.
 *
 * Gracias a eso, cambiar la matriz LED por un display de otro tipo no tocaria
 * ni una linea de este archivo.
 *
 * Reto 2 - Microcontroladores - Gildardo E. Restrepo - 2026-02
 */
#ifndef SYSTEM_FSM_H
#define SYSTEM_FSM_H

#include <stdint.h>
#include "keypad_fsm.h"

/** Que debe mostrarse. La MEF publica esto; no sabe como se dibuja. */
typedef enum {
    SYS_VIEW_IDLE = 0,   /**< Sistema listo para recibir una contrasena.   */
    SYS_VIEW_DIGIT,      /**< Digito recien pulsado.                       */
    SYS_VIEW_ENTERING,   /**< Progreso del ingreso.                        */
    SYS_VIEW_GRANTED,    /**< Acceso permitido.                            */
    SYS_VIEW_DENIED,     /**< Acceso denegado.                             */
    SYS_VIEW_LOCKED      /**< Sistema bloqueado. Reservado para la Fase 7. */
} SysView_t;

/** Peticion de pantalla completa. */
typedef struct {
    SysView_t view;
    uint8_t   digit;   /**< Valido solo si view == SYS_VIEW_DIGIT.     */
    uint8_t   count;   /**< Digitos ingresados. Para SYS_VIEW_ENTERING. */
} SysDisplay_t;

/** @brief Deja la maquina en ESPERA con el buffer vacio. */
void system_fsm_init(void);

/**
 * @brief Ejecuta un paso de la MEF.
 *
 * Debe llamarse en CADA tick, tambien cuando no hay evento: la maquina necesita
 * el tick para vigilar sus temporizadores.
 *
 * @param event Evento del teclado, o uno de tipo KEY_EVT_NONE.
 */
void system_fsm_step(KeyEvent_t event);

/** @brief Que debe mostrarse ahora mismo. */
SysDisplay_t system_fsm_get_display(void);

#endif /* SYSTEM_FSM_H */
