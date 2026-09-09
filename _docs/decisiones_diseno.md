---
title: Decisiones de diseño
created: 2026-09-09
time: 08:53am
creator: Gilbert
last update: 2026-09-09
update by: Gilbert
type: referencia
status: activo
fase: "1"
area: arquitectura de firmware
editor: Gilbert
order: 4
tags:
  - tipo/referencia
  - tipo/decisiones
---

# Decisiones de diseño

> [!success] Resumen
> Registro de **por qué** el firmware es como es. Cada entrada dice qué se
> decidió, qué alternativas había y qué se gana con la elegida. Es el material
> de respuesta para la sustentación.

---

## 1. Reloj del sistema: HSI a 16 MHz, sin PLL

**Decidido.** Se trabaja con el oscilador interno a 16 MHz, que es el reloj
activo por defecto tras un reset.

**Alternativa descartada:** configurar el PLL para llegar a 168 MHz.

**Justificación.** Activar el PLL obliga a tocar `RCC->PLLCFGR`, `RCC->CFGR` y,
sobre todo, `FLASH->ACR` para ajustar la latencia de la memoria. Configurar mal
esa latencia es una de las formas más habituales de dejar la placa colgada sin
mensaje de error. Para conmutar filas cada milisegundo, 16 MHz sobran: el
multiplexado consume unas decenas de instrucciones por tick sobre un presupuesto
de 16 000 ciclos.

**Efecto colateral favorable.** El proyecto no incluye `system_stm32f4xx.c`, así
que `SystemInit` no llega a ejecutarse (el enlazador convierte la llamada en un
`nop`). Como `SystemInit` tampoco toca el PLL, el resultado es coherente: el
reloj se queda donde lo asume el diseño.

---

## 2. Base de tiempo única: SysTick a 1 ms

**Decidido.** Un solo temporizador para todo el sistema, a 1 ms.

**Alternativa descartada:** un `TIM` dedicado para el refresco de la matriz.

**Justificación.** Con 1 ms se cubren a la vez las dos exigencias temporales del
reto sin necesidad de un segundo periférico:

| Tarea | Cadencia | Resultado |
|---|---|---|
| Matriz | 1 fila por tick | 125 Hz de refresco, sin parpadeo perceptible |
| Teclado | 1 semifase por tick | Barrido completo cada 8 ms |

Un `TIM` daría un refresco más estable frente al jitter del bucle, a cambio de
configurar más registros y de alejarse del *"el `while(1)` ejecuta las máquinas
de estados"* que describe el enunciado.

---

## 3. Detección de tick por contador, no por bandera

**Decidido.** `timebase_tick_ready()` compara el contador de milisegundos contra
el último valor atendido, e incrementa de uno en uno.

**Alternativa descartada:** una bandera booleana que la interrupción pone a 1 y
el bucle limpia.

**Justificación.** La bandera tiene dos defectos. Primero, una **condición de
carrera**: si `SysTick` interrumpe justo entre leer la bandera y limpiarla, ese
tick se pierde. Segundo, **no puede representar más de un tick acumulado**: si el
bucle llega tarde, los ticks atrasados desaparecen. Comparando contra el contador
e incrementando de uno en uno, el sistema se pone al día en las vueltas
siguientes y no pierde ninguno.

---

## 4. Escritura de pines siempre por `BSRR`

**Decidido.** Toda salida se escribe con `BSRR`, nunca con `ODR |= ...`.

**Justificación.** Tres razones, en orden de importancia para este proyecto:

1. Permite **subir unos pines y bajar otros en la misma instrucción**, que es lo
   que necesita el multiplexado para no generar estados intermedios visibles.
2. Es **atómico**: no hay ciclo leer-modificar-escribir que una interrupción
   pueda partir por la mitad.
3. **No toca los demás pines** del puerto, aunque no se conozca su estado.

---

## 5. Filas del teclado en open-drain

**Decidido.** Filas como salida open-drain, columnas como entrada con pull-up
interno. Lógica activa en bajo.

**Alternativa descartada:** filas push-pull.

**Justificación.** Con open-drain, escribir `1` no sube el pin: lo deja en alta
impedancia. Si el usuario pulsa dos teclas a la vez, con push-pull podría quedar
conectada una salida en alto contra otra en bajo, con la corriente limitada solo
por la resistencia de los propios drivers. Con open-drain esa situación no puede
darse. Cuesta lo mismo de programar: un bit en `OTYPER`.

---

## 6. Mapa de pines: cuatro buses contiguos

**Decidido.**

| Bus | Pines |
|---|---|
| Matriz columnas C1–C8 | `PD0`–`PD7` |
| Matriz filas F1–F8 | `PE8`–`PE15` |
| Teclado filas F0–F3 | `PB6`–`PB9` |
| Teclado columnas C0–C3 | `PA1`–`PA4` |

**Justificación.** Cada bus es un grupo contiguo dentro de su puerto, de modo que
se escribe con una sola operación sobre `BSRR` y su máscara es un byte o un
nibble desplazado. `PD0`–`PD7` es el byte bajo del puerto D y `PE8`–`PE15` el
byte alto del E.

**Revisión de la Fase 1.** La propuesta inicial situaba las filas de la matriz en
`PC0`–`PC7`. Al revisar el orden físico de los conectores 2x24 se vio que `PC0`
a `PC3` están seguidos pero después el header salta a `VREF-`, y que `PC4`–`PC8`
quedan repartidos por otras zonas. `PE8`–`PE15` sí salen como cuatro parejas
seguidas. **Lección: la contigüidad en la numeración del puerto no implica
contigüidad en el conector.**

Se descartó `PA0`–`PA7` por tres motivos: tampoco son contiguos en el header,
`PA6` y `PA7` llevan los LEDs D2 y D3 de la placa, y `PA0` es el botón WK_UP.

`PA1`–`PA4` se mantiene para las columnas del teclado pese a no ser contiguo en
el conector, por decisión explícita. Moverlo a `PC0`–`PC3` sería cambiar dos
`#define` en `board.h`.

---

## 7. Solo `stdint.h`

**Decidido.** Los únicos `#include` del proyecto son `<stdint.h>` y
`"stm32f4xx.h"`.

**Justificación.** Es la restricción de la asignatura, y encaja con el reto: sin
`string.h` la comparación de la contraseña se escribe a mano, sin `stdbool.h` los
booleanos son `uint8_t`, y sin `stdlib.h` no hay asignación dinámica. El
resultado es un binario cuyo contenido se entiende línea a línea.

---

## 8. Cada módulo configura sus propios pines

**Decidido.** No existe una función `board_init()` que configure todo. Cada
driver tiene su `_init()` y configura los pines que usa.

**Justificación.** Mantiene la responsabilidad donde está el conocimiento: quien
sabe que las filas del teclado deben ser open-drain es `keypad.c`, no un
inicializador central. Además permite probar los drivers de forma aislada, que es
justo lo que hacen las fases 2 y 3.

---

## 9. Matriz sin resistencias de limitación

**Decidido.** Los LED van directamente entre el pin de columna y el de fila, sin
resistencia en serie.

**Justificación.** Es el montaje disponible, y las pruebas no han dañado ningún
LED. La corriente queda limitada por la resistencia interna del driver de cada
pin (unos 25–40 Ω por lado) y el duty de 1/8 del multiplexado mantiene bajo el
promedio.

**Consecuencias, asumidas conscientemente:**

1. **Brillo dependiente del contenido.** Con un LED encendido en la fila activa
   circulan unos 20 mA; con ocho, el pin de fila los sume todos, su tensión sube
   y la corriente por LED cae a menos de la mitad. Una fila poco poblada se ve
   más brillante que una llena.
2. **Fuera de especificación.** El pico por el pin de fila queda por encima de
   los 25 mA que fija el datasheet. Funciona, pero es zona sin garantía del
   fabricante.

**Mitigación si hiciera falta:** ocho resistencias de 220–330 Ω en las columnas
uniformizan el brillo y devuelven el montaje al rango especificado, sin cambiar
una línea de código.

---

## 10. Tres interruptores de orientación en lugar de recablear

**Decidido.** `MATRIX_TRANSPOSE`, `MATRIX_ROW_REVERSE` y `MATRIX_COL_REVERSE` en
`board.h`, aplicados en `led_matrix_show()`.

**Justificación.** Con 16 hilos, la orientación real del montaje no se conoce
hasta encenderlo. Entre los tres flags se cubren las **ocho** orientaciones
posibles de una matriz 8x8, así que ningún resultado de las pruebas obliga a
recablear ni a reescribir bitmaps: se cambia un `0` por un `1`.

**Dónde se aplican y por qué ahí.** En `led_matrix_show()`, que se ejecuta al
cambiar de imagen, y no en `led_matrix_mux_step()`, que corre mil veces por
segundo. La transposición son 64 iteraciones que de este modo no cuestan nada.

**Resultado en la Fase 2.** Los tres quedaron en `0`: el montaje coincide con el
convenio de los bitmaps. Los fallos observados fueron de cableado (dos columnas
intercambiadas y una fila mal conectada), no de orientación.

---

## 11. El driver del teclado no sabe qué significa cada tecla

**Decidido.** `keypad` publica un índice de 0 a 15 (`fila * 4 + columna`) y
nada más. La traducción a carácter (`1`, `A`, `*`…) vive en la capa de
aplicación, en la Fase 5.

**Justificación.** Mantiene el driver en el nivel de hardware puro. Si mañana se
cambia el teclado por uno con otra serigrafía, se toca una tabla de la
aplicación y ni una línea del driver. Tiene además una ventaja práctica: si al
cablear resultan cruzadas las filas y las columnas, el efecto es solo que los
índices salen transpuestos, y se corrige en esa misma tabla.

**Corolario.** El driver tampoco filtra rebotes. Publica lo que hay en el
teclado *ahora*; interpretar si eso es una pulsación válida es trabajo de la MEF
de antirrebote.

---

## 12. La instantánea del teclado se publica solo al final del barrido

**Decidido.** `keypad_get_raw_key()` se actualiza al terminar las cuatro filas,
cada 8 ms, y no fila a fila.

**Justificación.** Publicar a mitad de barrido dejaría ver un estado que mezcla
dos momentos distintos: filas ya exploradas del ciclo nuevo junto a filas del
anterior. El consumidor siempre lee una foto coherente del teclado completo.

---

## 13. El antirrebote avanza al ritmo del barrido, no del tick

**Decidido.** `keypad_fsm_step()` se llama en cada tick pero solo progresa
cuando `keypad_snapshot_ready()` confirma que hay una instantánea nueva.

**Alternativa descartada:** contar milisegundos con un `SwTimer` dentro de la
propia MEF de antirrebote.

**Justificación.** El driver publica una foto cada 8 ms. Si la máquina avanzara
en cada tick de 1 ms leería ocho veces seguidas exactamente el mismo valor, y
las cuatro muestras estables serían 4 ms en lugar de 24: no filtraría nada.
Consumiendo la bandera del driver, **cada muestra es una lectura genuinamente
nueva** y el conteo equivale directamente a tiempo, sin introducir un segundo
reloj para medir lo mismo.

---

## 14. `KEY_PRESSED` como estado de paso

**Decidido.** El estado que emite `KEY_EVT_PRESSED` se atraviesa dentro del
mismo paso en que se alcanza y nunca sobrevive al siguiente.

**Justificación.** Es la forma de hacer que *"un evento por pulsación"* sea una
propiedad **estructural** y no una convención. `KEY_PRESSED` es el único punto
del programa que emite ese evento, y no existe ningún camino en la máquina que
vuelva a pasar por él sin haber pasado antes por `KEY_IDLE`. Aunque la capa de
aplicación se olvidara de filtrar, seguiría siendo imposible recibir el evento
dos veces.

Es también la respuesta corta en la sustentación a *"¿cómo garantizas que una
tecla mantenida no genera cientos de eventos?"*.

---

## 15. Una segunda tecla se trata como liberación

**Decidido.** Estando en `KEY_HELD`, cualquier lectura distinta de la tecla
mantenida arranca el antirrebote de suelta, incluida la aparición de otra tecla.

**Justificación.** Obliga a la nueva tecla a pasar por `KEY_IDLE` y por su
propio antirrebote antes de considerarse válida. Es más predecible que intentar
resolver la multitecla, que el reto no pide, y evita que una pulsación se cuele
sin haber sido confirmada.

---

## 16. La contraseña vive en FLASH

**Decidido.** `static const uint8_t k_stored_password[4] = { 8, 1, 9, 1 };`

**Alternativa descartada:** un arreglo en RAM, modificable en ejecución.

**Justificación.** Al ser `const`, el enlazador la coloca en `.rodata`, dentro de
la FLASH. Verificado sobre el `.elf`: queda en `0x080011A4`. Ninguna ejecución
puede alterarla: no hay puntero que la pise ni desbordamiento de pila que la
corrompa. En RAM sería modificable en caliente, pero se perdería en cada reset y
el enunciado no pide cambiarla.

**Limitación asumida.** No hay cifrado, tal y como permite el enunciado:
cualquiera con acceso al `.hex` puede leer la clave. Es una decisión consciente,
no un descuido.

---

## 17. La comparación no sale al primer fallo

**Decidido.** `password_matches()` recorre siempre las cuatro posiciones y
acumula el resultado, en lugar de retornar en cuanto encuentra una diferencia.

**Justificación.** Un `return` anticipado haría que el tiempo de ejecución
dependiera de cuántos dígitos iniciales fueran correctos, que es la base de un
ataque por temporización. Aquí nadie está midiendo microsegundos, pero es la
forma correcta de escribirlo y no cuesta nada: cuatro iteraciones frente a una.

---

## 18. `ST_VALIDANDO` como estado de paso

**Decidido.** Misma técnica que `KEY_PRESSED` en la MEF de antirrebote: se
atraviesa dentro del mismo paso en que se completa el cuarto dígito.

**Justificación.** Hace que **validar dos veces el mismo ingreso sea
imposible por construcción**, no por convención. Es además el único punto del
programa que compara la contraseña.

---

## 19. Reparto de las 16 teclas

**Decidido.** Las diez numéricas ocupan una posición del buffer. `*` borra el
ingreso y devuelve a ESPERA. `A`, `B`, `C`, `D` y `#` se ignoran por completo.

**Justificación.** La contraseña es numérica, así que aceptar letras como
entrada obligaría a comparar por índice de tecla en lugar de por dígito, y a
dibujar cinco glifos más que no aportan nada. Ignorarlas no rompe un ingreso en
curso, que es el comportamiento menos sorprendente para el usuario.

---

## 20. La MEF de la aplicación publica vistas, no dibujos

**Decidido.** `system_fsm` publica un `SysDisplay_t` con un enumerado de vista.
La traducción de vista a mapa de bits ocurre en `render()`, dentro de `main.c`.

**Justificación.** `system_fsm` no sabe que existe una matriz de LED, y `main`
no sabe que existe una contraseña. Cambiar el display por otro dispositivo no
tocaría una sola línea de la lógica del producto, y cambiar el aspecto del
sistema es tocar solo `render()` y los bitmaps.

---

## 21. LED D2 solo para diagnóstico

**Decidido.** `PA6` se usa en la Fase 1 para validar la cadena de compilación y
flasheo, y desaparece del sistema final.

**Justificación.** Separa el fallo de *toolchain* del fallo de *aplicación*. Si
en una fase posterior algo no funciona, un parpadeo de D2 confirma en dos
segundos que el binario cargó y que la base de tiempo corre.

**Retirado en la Fase 5.** Cumplida su función, D2 sale del `main`: no forma
parte del sistema que pide el enunciado, y la imagen de espera ya indica por sí
sola que el multiplexado sigue corriendo.

---
## Enlaces
[[proyectos|Proyectos Electrónica]]
