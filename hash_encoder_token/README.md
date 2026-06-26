# hash_encoder_token

Modulo de comunicacion USB CDC ACM para la STM32F429I Discovery.

## Archivos

- `stm_comm.c` / `stm_comm.h`: modulo para inicializar, mandar y recibir datos.
- `main.c`: ejemplo de uso con comandos simples.
- `Makefile`: build del ejemplo para la F429I Discovery.

## Uso desde la PC

Despues de flashear, la tarjeta aparece como un puerto serial USB, normalmente
`/dev/ttyACM0`.

```sh
make
make flash
screen /dev/ttyACM0 115200
```

Comandos de prueba:

- `uid`: imprime el UID unico de 96 bits del STM32 en hexadecimal.
- `hash <texto>`: calcula el SHA1 completamente por software.
- `hash_test`: verifica la implementacion por software con el vector conocido
  `SHA1("abc")`.
- `bloqueo`: entra en modo bloqueo, manda `bloqueo` constantemente y hace
  parpadear el LED rojo LD4.
- `1234`: cuando esta bloqueado, desbloquea y hace parpadear el LED verde LD3
  20 veces.

Cada tecla recibida se muestra en la terminal como la letra real escrita.

Ejemplo:

```text
hash gato
SHA1(gato) = 8f39c63d50478f69b087a9696546e72e50cd1967
Tiempo SHA1: <ciclos> (<microsegundos> us)
hash_test
SHA1 software test: OK
```

El tiempo se mide con el contador de ciclos DWT del Cortex-M4. La medicion
incluye solamente el calculo SHA1; no incluye la recepcion ni el envio por USB.

La contrasena se cambia en `main.c`, en:

```c
#define UNLOCK_PASSWORD "1234"
```
