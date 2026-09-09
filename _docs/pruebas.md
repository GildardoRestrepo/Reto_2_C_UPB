---
title: Plan y bitácora de pruebas
created: 2026-09-09
time: 08:53am
creator: Gilbert
last update: 2026-09-09
update by: Gilbert
type: referencia
status: activo
fase: "1"
area: verificación
editor: Gilbert
order: 5
tags:
  - tipo/referencia
  - tipo/pruebas
---

# Plan y bitácora de pruebas

> [!success] Resumen
> Dos cosas: el **plan** de pruebas de aceptación que exige el enunciado (§21) y
> la **bitácora** de lo verificado al cerrar cada fase. Sirve de evidencia de
> proceso y de guion para la demostración.

---

## 1. Pruebas de aceptación del enunciado

Son las nueve pruebas mínimas que pide el reto. Se marcan a medida que las fases
las habilitan.

| #   | Prueba                        | Resultado esperado                         | Fase que la habilita | Estado |
| --- | ----------------------------- | ------------------------------------------ | -------------------- | ------ |
| 1   | Encender el sistema           | Aparece la imagen de espera                | Fase 5               | ⬜      |
| 2   | Presionar una tecla           | Se detecta una única pulsación             | Fase 4               | ✅      |
| 3   | Mantener una tecla presionada | No se generan múltiples pulsaciones        | Fase 4               | ✅      |
| 4   | Liberar una tecla             | El sistema vuelve a aceptar pulsaciones    | Fase 4               | ✅      |
| 5   | Ingresar 4 dígitos            | Se inicia la validación                    | Fase 5               | ⬜      |
| 6   | Contraseña correcta           | Aparece la imagen de acceso                | Fase 5               | ⬜      |
| 7   | Contraseña incorrecta         | Aparece la imagen de error                 | Fase 5               | ⬜      |
| 8   | Esperar 3 s                   | Desaparece la imagen de resultado          | Fase 5               | ⬜      |
| 9   | Nueva contraseña              | El sistema permite un nuevo intento        | Fase 5               | ⬜      |
| 10  | Funcionamiento continuo       | La matriz no parpadea de forma perceptible | Fase 2               | ✅      |

Prueba adicional del reto opcional:

| # | Prueba | Resultado esperado | Fase | Estado |
|---|---|---|---|---|
| 11 | Tres contraseñas incorrectas seguidas | El sistema entra en bloqueo temporal y sale solo | Fase 7 | ⬜ |

---

## 2. Bitácora por fase

### Fase 1 — Base de tiempo ✅

**Fecha:** 2026-09-09
**Rama:** `fase-1-base-de-tiempo`
**Objetivo:** validar la cadena CubeIDE → `.hex` → CubeProgrammer → placa, y la
base de tiempo de 1 ms.

#### Verificación estática (antes de flashear)

Se compiló y enlazó desde consola con el toolchain del propio CubeIDE
(`arm-none-eabi-gcc 14.3.1`) y los flags exactos del proyecto:

| Comprobación | Resultado |
|---|---|
| Compilación con `-Wall -Wextra -Wconversion -Wshadow -Wundef` | Sin errores ni warnings |
| Enlazado con `STM32F407VETX_FLASH.ld` | Correcto |
| Tamaño | `text 1508 B` · `data 0 B` · `bss 1576 B` |
| Vector `0x00` (stack inicial) | `0x20020000`, coincide con `_estack` |
| Vector `0x04` (Reset) | Apunta a `Reset_Handler` |
| **Vector `0x3C` (SysTick)** | `0x0800032D`, apunta a nuestro `SysTick_Handler` |
| Instrucciones de FPU en el binario | Ninguna |

> [!note] Sobre `SystemInit`
> El proyecto no incluye `system_stm32f4xx.c`. El `startup` hace
> `bl SystemInit` con el símbolo declarado `.weak`, y el enlazador convierte esa
> llamada en un `nop.w`. No hay cuelgue. Como no se usan instrucciones de FPU, el
> hecho de que `SystemInit` no habilite `CPACR` es irrelevante, y como tampoco
> configura el PLL, el reloj se queda en HSI a 16 MHz tal y como asume el diseño.

#### Verificación en hardware

| #   | Prueba                                     | Resultado esperado                      | Estado |
| --- | ------------------------------------------ | --------------------------------------- | ------ |
| 1.1 | El proyecto compila en CubeIDE             | Build sin errores                       | ✅      |
| 1.2 | Se genera el `.hex`                        | `Debug/reto_2_C_microcontroladores.hex` | ✅      |
| 1.3 | Flasheo en `0x08000000` con CubeProgrammer | Programación correcta                   | ✅      |
| 1.4 | Tras el reset, D2 parpadea                 | Parpadeo visible y regular              | ✅      |
| 1.5 | Período del parpadeo                       | 1 Parpadeo por segundo                  | ✅      |

#### Conclusión

Queda validado el flujo de trabajo completo y las dos capas base (`gpio` y
`timebase`) sobre las que se apoyan las cinco máquinas de estados. La estructura
del `while(1)` ya es la definitiva.

---

### Fase 2 — Driver de la matriz LED ✅

**2026-09-09** · rama `fase-2-driver-matriz-led`

| # | Prueba | Resultado |
|---|---|---|
| 2.1 | Multiplexado a 125 Hz | Sin parpadeo perceptible ✅ |
| 2.2 | Blanking | Sin ghosting ✅ |
| 2.3 | Orientación (`img_test_f`) | Correcta, los tres flags quedan en `0` ✅ |
| 2.4 | Cableado (`img_test_border`, `img_test_all`) | Detectó dos columnas intercambiadas y una fila mal conectada; corregidas ✅ |
| 2.5 | No bloqueo | Las imágenes rotan cada 2 s sin interrumpir el refresco ✅ |

Incógnitas cerradas: **ánodo común en las columnas** y **sin resistencias**
(justificación en `decisiones_diseno.md` §9).

Aprendizaje: las imágenes de diagnóstico deben ser densas. La F tiene píxeles
apagados por diseño y se confundieron con fallos; `img_test_all` localizó los
pines mal conectados de un vistazo.

### Fase 3 — Driver del teclado 4x4 ✅

**2026-09-09** · rama `fase-3-driver-teclado`

| # | Prueba | Resultado |
|---|---|---|
| 3.1 | Barrido IN-OUT de las 16 teclas | Cada tecla enciende su bloque 2x2 en la posición correcta ✅ |
| 3.2 | Cobertura de la rejilla | Las 16 teclas tesela la matriz sin huecos ni solapes ✅ |
| 3.3 | Reposo | Vuelve a la imagen de esquinas al soltar ✅ |
| 3.4 | No bloqueo | El barrido no interrumpe el refresco de la matriz ✅ |
| 3.5 | Rebote | Visible al pulsar, **como se esperaba**: lo resuelve la Fase 4 |

Barrido a dos fases (activar en un tick, leer en el siguiente): 8 ms por barrido
completo, sin ninguna espera activa.

### Fase 4 — MEF de antirrebote ✅

**2026-09-09** · rama `fase-4-antirrebote`

| # | Prueba | Resultado |
|---|---|---|
| 4.1 | Una pulsación → un evento | D2 conmuta una sola vez ✅ |
| 4.2 | Tecla mantenida 5 s | Un único cambio de D2, sin repeticiones ✅ |
| 4.3 | Diez pulsaciones | Diez conmutaciones, ni una más ✅ |
| 4.4 | Liberación | Vuelve a la imagen de reposo y acepta nuevas pulsaciones ✅ |
| 4.5 | Rebote residual | Desaparece el parpadeo que se veía en la Fase 3 ✅ |

Verificación previa por simulación de la MEF: pulsación con rebote sucio, tecla
mantenida 8 s, rebote que nunca se estabiliza, segunda tecla sin soltar la
primera y diez pulsaciones limpias. Los cinco casos dan el número exacto de
eventos esperado.

Umbral: 4 muestras separadas 8 ms = **24 ms**. Latencia de respuesta hasta 32 ms.

---

## Enlaces
[[proyectos|Proyectos Electrónica]]
