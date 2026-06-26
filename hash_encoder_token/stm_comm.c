#include "stm_comm.h"

#include <libopencm3-plus/newlib/devices/cdcacm.h>
#include <libopencm3-plus/newlib/syscall.h>

#include <stdio.h>
#include <string.h>

#define STM_COMM_USB_PACKET_SIZE 64

static long stm_comm_usb_write(int fd, const char *data, int length)
{
	int sent = 0;

	(void)fd;

	/*
	 * El endpoint CDC acepta como maximo 64 bytes por transferencia.
	 * Dividir aqui tambien protege las escrituras largas hechas por printf().
	 */
	while (sent < length) {
		int remaining = length - sent;
		int chunk = remaining > STM_COMM_USB_PACKET_SIZE
				    ? STM_COMM_USB_PACKET_SIZE
				    : remaining;

		cdcacm_write_now(data + sent, chunk);
		sent += chunk;
	}

	return length;
}

static devoptab_t stm_comm_usb_devoptab = {
	"cdcacm",
	cdcacm_open,
	cdcacm_close,
	stm_comm_usb_write,
	cdcacm_read,
	cdcacm_in_poll,
};

void stm_comm_init(void)
{
	/*
	 * La libreria usa devoptab_list para decidir a que dispositivo van
	 * stdin, stdout y stderr. Aqui los tres apuntan al USB CDC ACM.
	 */
	devoptab_list[0] = &stm_comm_usb_devoptab;
	devoptab_list[1] = &stm_comm_usb_devoptab;
	devoptab_list[2] = &stm_comm_usb_devoptab;

	/* Inicializa el USB del STM32F429I Discovery y espera a que la PC enumere. */
	cdcacm_f429_init();

	/* Desactiva buffering para que los datos salgan y entren de inmediato. */
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	/* Limpia bytes viejos que pudieron quedar pendientes al conectar. */
	while (stm_comm_available() > 0) {
		(void)getc(stdin);
	}
}

void stm_comm_send(const char *message)
{
	/* Evita intentar enviar desde un puntero invalido. */
	if (message == NULL) {
		return;
	}

	/* Reutiliza la funcion de bytes calculando el largo del texto. */
	stm_comm_send_bytes((const uint8_t *)message, strlen(message));
}

void stm_comm_send_bytes(const uint8_t *data, size_t length)
{
	/* Si no hay buffer, no hay nada seguro que mandar. */
	if (data == NULL) {
		return;
	}

	/* putc escribe en stdout, que en stm_comm_init se conecto al USB CDC. */
	for (size_t i = 0; i < length; i++) {
		putc(data[i], stdout);
	}
}

int stm_comm_available(void)
{
	/* lo_poll pregunta cuantos bytes estan esperando en stdin. */
	return lo_poll(stdin);
}

bool stm_comm_read_byte(uint8_t *byte)
{
	/* Solo lee si el usuario paso donde guardar y si hay datos disponibles. */
	if (byte == NULL || stm_comm_available() <= 0) {
		return false;
	}

	/* getc lee desde stdin, que esta conectado al USB CDC. */
	*byte = (uint8_t)getc(stdin);
	return true;
}

size_t stm_comm_read_line(char *buffer, size_t buffer_size)
{
	/*
	 * Guarda el avance entre llamadas.
	 *
	 * El main llama esta funcion muchas veces por segundo. Normalmente los
	 * caracteres no llegan todos al mismo tiempo, entonces hay que recordar
	 * lo recibido hasta que llegue Enter.
	 */
	static size_t length;
	static bool overflow;
	static bool skip_next_lf;

	if (buffer == NULL || buffer_size == 0) {
		return 0;
	}

	while (stm_comm_available() > 0) {
		int c = getc(stdin);

		if (skip_next_lf) {
			skip_next_lf = false;
			if (c == '\n') {
				continue;
			}
		}

		/* En terminales seriales Enter puede llegar como '\r', '\n' o ambos. */
		if (c == '\r' || c == '\n') {
			if (c == '\r') {
				skip_next_lf = true;
			}

			putc('\r', stdout);
			putc('\n', stdout);

			if (overflow) {
				length = 0;
				overflow = false;
				buffer[0] = '\0';
				continue;
			}

			/* Ignora enters sueltos antes de recibir contenido real. */
			if (length == 0) {
				buffer[0] = '\0';
				continue;
			}

			buffer[length] = '\0';
			size_t line_length = length;
			length = 0;
			return line_length;
		}

		/* Muestra en la terminal el caracter real que se esta escribiendo. */
		putc(c, stdout);

		/* Guarda solo si aun queda espacio para el '\0' final. */
		if (length < (buffer_size - 1)) {
			buffer[length++] = (char)c;
		} else {
			/*
			 * Si la palabra es mas grande que el buffer, se descarta el
			 * resto hasta Enter para no entregar una linea cortada.
			 */
			overflow = true;
		}
	}

	/* Todavia no llego Enter, entonces no hay linea completa lista. */
	buffer[length] = '\0';
	return 0;
}
