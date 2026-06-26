#ifndef STM_COMM_H
#define STM_COMM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Inicializa la comunicacion USB CDC ACM.
 *
 * Despues de llamar esta funcion, stdin/stdout/stderr quedan conectados al
 * puerto serial USB que ve la PC como /dev/ttyACM*.
 */
void stm_comm_init(void);

/* Envia una cadena de texto terminada en '\0'. */
void stm_comm_send(const char *message);

/* Envia un bloque de bytes. Sirve para texto o datos binarios. */
void stm_comm_send_bytes(const uint8_t *data, size_t length);

/* Retorna cuantos caracteres hay disponibles para leer desde la PC. */
int stm_comm_available(void);

/*
 * Lee un byte si hay datos disponibles.
 *
 * Retorna true si pudo leer y guarda el byte en *byte.
 * Retorna false si no habia datos o si byte era NULL.
 */
bool stm_comm_read_byte(uint8_t *byte);

/*
 * Lee una linea recibida por USB hasta encontrar Enter.
 *
 * La linea queda terminada con '\0'. Retorna la cantidad de caracteres leidos,
 * sin contar el terminador. Si no hay una linea disponible, retorna 0.
 */
size_t stm_comm_read_line(char *buffer, size_t buffer_size);

#endif
