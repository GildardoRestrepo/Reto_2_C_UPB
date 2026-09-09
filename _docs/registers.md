---
title: Registros utilizados
created: 2026-09-08
time: 11:41pm
creator: Gilbert
last update: 2026-09-08
update by: Gilbert
type: referencia
status: activo
fase: "0"
area: firmware bare-metal
editor: Gilbert
order: 2
tags:
  - tipo/referencia
  - tipo/registros
---

# Registros utilizados

> [!success] Resumen
> Todos los registros que toca el proyecto, con su dirección, sus campos y el
> **valor concreto** que se les escribe según el mapa de pines acordado.
> Sirve de referencia al escribir el código y de soporte en la sustentación.

---

## 1. Reglas de la implementación

> [!important]  
> - `<stdint.h>` — tipos de ancho fijo (`uint8_t`, `uint32_t`).
> - `"stm32f4xx.h"` — definiciones de registros del fabricante (CMSIS).


> [!note] Sobre `volatile`
> Los registros ya están declarados `volatile` dentro de las estructuras de CMSIS
> (`__IO uint32_t`). No hay que añadirlo. Sí hace falta en las variables
> compartidas entre la interrupción de `SysTick` y el `while(1)`.

---

## 2. Mapa de direcciones

Los GPIO del STM32F407 cuelgan del bus **AHB1**.

| Periférico | Dirección base |
|---|---|
| `AHB1PERIPH_BASE` | `0x4002 0000` |
| `GPIOA` | `0x4002 0000` |
| `GPIOB` | `0x4002 0400` |
| `GPIOC` | `0x4002 0800` |
| `GPIOD` | `0x4002 0C00` |
| `GPIOE` | `0x4002 1000` |
| `RCC` | `0x4002 3800` |
| `SysTick` | `0xE000 E010` |

Cada puerto GPIO ocupa `0x400` bytes. `SysTick` no es un periférico del
fabricante sino del núcleo Cortex-M4, por eso vive en la zona `0xE000 xxxx`.

---

## 3. RCC — habilitación de relojes

### `RCC->AHB1ENR` (offset `0x30`)

Tras el reset **todos los relojes de periférico están apagados**. Escribir en un
registro de GPIO sin haber habilitado su reloj no produce ningún error: la
escritura simplemente se pierde. Es el error número uno al empezar en bare-metal.

| Bit | Nombre | Efecto |
|---|---|---|
| 0 | `GPIOAEN` | Habilita el reloj de `GPIOA` |
| 1 | `GPIOBEN` | Habilita el reloj de `GPIOB` |
| 2 | `GPIOCEN` | Habilita el reloj de `GPIOC` |
| 3 | `GPIODEN` | Habilita el reloj de `GPIOD` |
| 4 | `GPIOEEN` | Habilita el reloj de `GPIOE` |

**Valor del proyecto:** se usan los puertos A, B, D y E. El puerto C no se usa.

```c
RCC->AHB1ENR |= 0x0000001Bu;   /* GPIOA | GPIOB | GPIOD | GPIOE */
(void)RCC->AHB1ENR;            /* lectura de vuelta: asegura que ya esta activo */
```

`GPIOA` solo hace falta por las columnas del teclado (`PA1`–`PA4`) y, en la
Fase 1, por el LED de diagnóstico `PA6`.

La lectura de vuelta existe porque la habilitación tarda unos ciclos en
propagarse por el bus. Sin ella, la instrucción siguiente puede escribir un
registro que todavía no responde.

---

## 4. GPIO — registros de un puerto

Offsets relativos a la base del puerto:

| Offset | Registro | Ancho por pin | Para qué |
|---|---|---|---|
| `0x00` | `MODER` | 2 bits | Entrada / salida / alternativa / analógico |
| `0x04` | `OTYPER` | 1 bit | Push-pull u open-drain |
| `0x08` | `OSPEEDR` | 2 bits | Velocidad de flanco |
| `0x0C` | `PUPDR` | 2 bits | Pull-up / pull-down internos |
| `0x10` | `IDR` | 1 bit | **Lectura** del estado del pin |
| `0x14` | `ODR` | 1 bit | Lectura/escritura del latch de salida |
| `0x18` | `BSRR` | 2x16 bits | **Escritura atómica** de set y reset |

### Codificación de los campos

| Registro | Valor | Significado |
|---|---|---|
| `MODER` | `00` | Entrada |
| | `01` | Salida de propósito general |
| | `10` | Función alternativa |
| | `11` | Analógico |
| `OTYPER` | `0` | Push-pull |
| | `1` | Open-drain |
| `PUPDR` | `00` | Sin resistencia |
| | `01` | Pull-up |
| | `10` | Pull-down |
| `OSPEEDR` | `00` | Baja (suficiente para este proyecto) |

Para un pin `n`, el desplazamiento en los registros de 2 bits es `2 * n`, y en
los de 1 bit es `n`. De ahí el patrón que se repite en todo el código:

```c
port->MODER &= ~(0x3u << (2u * pin));   /* limpiar el campo   */
port->MODER |=  (valor << (2u * pin));  /* escribir el nuevo  */
```

Primero se limpia y luego se escribe. Un `|=` sin limpiar previamente **no puede
cambiar un 1 a 0**, así que reconfigurar un pin dejaría bits del modo anterior.

### `BSRR` — por qué se usa en lugar de `ODR`

`BSRR` tiene 32 bits divididos en dos mitades:

- Bits `0..15` (**BS**): escribir 1 **pone a 1** ese pin.
- Bits `16..31` (**BR**): escribir 1 **pone a 0** ese pin.
- Escribir 0 en cualquier bit no hace nada.

Ventajas frente a `ODR |= ...`:

1. **Es atómico**: una sola escritura, sin el ciclo leer-modificar-escribir. Si
   una interrupción cae en medio de un `ODR |=`, el resultado puede corromperse.
2. **No toca los demás pines** del puerto, aunque no se sepa su estado.
3. Permite **subir unos pines y bajar otros en la misma instrucción**, que es
   justo lo que necesita el multiplexado para no generar estados intermedios.

La primitiva del proyecto:

```c
/* Escribe 'value' solo en los pines de 'mask', en una sola operacion. */
port->BSRR = ((mask & ~value) << 16u) | (mask & value);
```

---

## 5. Valores concretos por bus

### Matriz LED — columnas C1–C8 en `PD0`–`PD7` (salida push-pull)

| Registro | Máscara a limpiar | Valor a escribir |
|---|---|---|
| `GPIOD->MODER` | `0x0000FFFF` | `0x00005555` |
| `GPIOD->OTYPER` | `0x000000FF` | `0x00000000` |
| `GPIOD->PUPDR` | `0x0000FFFF` | `0x00000000` |

`0x5555` es `01` repetido ocho veces: los ocho pines en modo salida.

### Matriz LED — filas F1–F8 en `PE8`–`PE15` (salida push-pull)

Al estar en el **byte alto** del puerto, los campos caen en la mitad superior de
los registros de 2 bits:

| Registro | Máscara a limpiar | Valor a escribir |
|---|---|---|
| `GPIOE->MODER` | `0xFFFF0000` | `0x55550000` |
| `GPIOE->OTYPER` | `0x0000FF00` | `0x00000000` |
| `GPIOE->PUPDR` | `0xFFFF0000` | `0x00000000` |

Es el mismo `0x5555` de las columnas, desplazado 16 bits: los pines 8 a 15 ocupan
los bits 16 a 31 de `MODER`.

> [!important] Polaridad, verificada en la Fase 2
> La matriz es de **ánodo común en las columnas**:
> `PDx` (columna, ánodo) → LED → `PEy` (fila, cátodo).
> Por tanto **las columnas son activas en alto y las filas activas en bajo**, y
> la fila activa es el sumidero de todos los LED encendidos de esa fila.

### Teclado — filas F0–F3 en `PB6`–`PB9` (salida open-drain)

| Registro | Máscara a limpiar | Valor a escribir |
|---|---|---|
| `GPIOB->MODER` | `0x000FF000` | `0x00055000` |
| `GPIOB->OTYPER` | `0x000003C0` | `0x000003C0` |

Los cuatro pines quedan en open-drain: escribir `0` lleva la fila a masa,
escribir `1` la deja en **alta impedancia**. Por eso pulsar dos teclas a la vez
no cortocircuita dos salidas entre sí.

### Teclado — columnas C0–C3 en `PA1`–`PA4` (entrada con pull-up)

| Registro | Máscara a limpiar | Valor a escribir |
|---|---|---|
| `GPIOA->MODER` | `0x000003FC` | `0x00000000` |
| `GPIOA->PUPDR` | `0x000003FC` | `0x00000154` |

`0x154` es `01` en las posiciones de los pines 1, 2, 3 y 4. Con pull-up, en
reposo se lee `1`; una columna a `0` significa **tecla pulsada** (activo en bajo).

### LED de diagnóstico `PA6` (salida push-pull, activo en bajo)

| Registro | Máscara a limpiar | Valor a escribir |
|---|---|---|
| `GPIOA->MODER` | `0x00003000` | `0x00001000` |

---

## 6. Operaciones en tiempo de ejecución

### Multiplexar una fila de la matriz

Tres escrituras, **en este orden**:

```c
/* 1. Blanking: apagar todas las columnas antes de cambiar de fila. */
GPIOD->BSRR = 0x00FF0000u;

/* 2. Seleccionar la fila r.
 *    Las filas son CATODOS: activas en BAJO. Hay que poner las siete inactivas
 *    a 1 y la activa a 0, todo en la misma escritura.
 *    OJO: la fila activa no puede aparecer tambien en la mitad de set, porque
 *    si un pin lleva a 1 su bit BS y su bit BR a la vez, gana el set. */
const uint32_t row_bit = (1u << (8u + r));
GPIOE->BSRR = (0xFF00u & ~row_bit) | (row_bit << 16u);

/* 3. Volcar el patron de esa fila en las columnas. */
GPIOD->BSRR = ((uint32_t)(~patron & 0xFFu) << 16u) | patron;
```

Sin el paso 1 aparece **ghosting**: durante el instante entre cambiar de fila y
escribir las nuevas columnas, la fila nueva se ilumina con los datos de la
anterior y se ve un fantasma de la fila previa.

### Barrer una fila del teclado

```c
/* Activar solo la fila r (bajarla) y dejar las otras tres en alta impedancia. */
const uint32_t fila = 1u << (6u + r);
GPIOB->BSRR = ((0x03C0u & ~fila)) | (fila << 16u);

/* Un tick despues, leer las cuatro columnas. Un 0 indica tecla pulsada. */
const uint32_t columnas = (~GPIOA->IDR >> 1u) & 0x0Fu;  /* 1 = pulsada */
```

Se lee de `IDR`, nunca de `ODR`: `IDR` refleja el **estado real del pin**,
mientras que `ODR` solo devuelve lo último que se escribió.

---

## 7. SysTick — base de tiempo de 1 ms

Registros del núcleo, base `0xE000 E010`:

| Offset | Registro | Función |
|---|---|---|
| `0x00` | `CTRL` | Habilitación, interrupción y fuente de reloj |
| `0x04` | `LOAD` | Valor de recarga, 24 bits |
| `0x08` | `VAL` | Contador actual; escribir cualquier valor lo pone a 0 |
| `0x0C` | `CALIB` | Calibración de fábrica (no se usa) |

### `SysTick->CTRL`

| Bit | Nombre | Valor usado |
|---|---|---|
| 0 | `ENABLE` | `1` — arranca el contador |
| 1 | `TICKINT` | `1` — genera excepción al llegar a 0 |
| 2 | `CLKSOURCE` | `1` — reloj del procesador (16 MHz), no dividido por 8 |
| 16 | `COUNTFLAG` | solo lectura; se limpia al leerlo |


### Cálculo de la recarga

El contador es **descendente** y genera la excepción al pasar de 1 a 0, por eso
se resta uno:

```
LOAD = (f_reloj / f_tick) - 1 = (16 000 000 / 1000) - 1 = 15 999 = 0x3E7F
```

```c
SysTick->LOAD = 15999u;      /* 1 ms a 16 MHz */
SysTick->VAL  = 0u;          /* limpiar el contador y COUNTFLAG */
SysTick->CTRL = 0x00000007u; /* CLKSOURCE | TICKINT | ENABLE */
```

`LOAD` es de 24 bits, así que el máximo es `0xFFFFFF`: a 16 MHz, un periodo de
hasta **1,048 s**. Más que suficiente para 1 ms, y la razón por la que los 3 s
del enunciado se cuentan acumulando ticks en software y no en el propio `SysTick`.


### Manejador

`SysTick_Handler` ya existe como símbolo débil en el `startup`. Al definirlo en
nuestro código, el enlazador sustituye el manejador por defecto. **No hay que
tocar el NVIC**: `SysTick` es una excepción del núcleo, no una interrupción de
periférico.

```c
void SysTick_Handler(void)
{
    s_millis++;          /* contador de milisegundos */
    s_tick_flag = 1u;    /* aviso al while(1) */
}
```

Ambas variables se declaran `volatile`: las modifica la interrupción y las lee el
bucle principal, así que el compilador no puede cachearlas en un registro.



## 8. Registros que este proyecto NO toca

| Registro | Por qué no |
|---|---|
| `RCC->PLLCFGR`, `RCC->CFGR`, `FLASH->ACR` | Se trabaja con el **HSI a 16 MHz**, que ya está activo tras el reset. Sin PLL no hay latencia de FLASH que ajustar. |
| `NVIC->ISER`, `NVIC->IPR` | La única interrupción es `SysTick`, que es una excepción del núcleo y se habilita desde su propio `CTRL`. |
| `GPIOx->AFRL`, `GPIOx->AFRH` | Ningún pin usa función alternativa: todos son GPIO puros. |
| `TIMx`, `EXTI`, `SYSCFG` | La base de tiempo única es `SysTick` y el teclado se lee por barrido, no por interrupción externa. |


---
## Enlaces
[[proyectos|Proyectos Electrónica]]
