#include "stm_comm.h"
#include "sha1_sw.h"

#include <libopencm3/cm3/dwt.h>
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LED_GREEN_PORT GPIOG
#define LED_GREEN_PIN GPIO13
#define LED_RED_PORT GPIOG
#define LED_RED_PIN GPIO14

#define UNLOCK_PASSWORD "1234"
#define BLOCK_PRINT_PERIOD_MS 500
#define LED_BLINK_PERIOD_MS 250
#define UNLOCK_GREEN_BLINKS 20

typedef enum {
	APP_NORMAL,
	APP_BLOCKED,
	APP_UNLOCKED_BLINK,
} app_state_t;

static bool cycle_counter_available;

static void clock_init(void)
{
	rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
}

static void leds_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOG);

	gpio_mode_setup(LED_GREEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
			LED_GREEN_PIN);
	gpio_mode_setup(LED_RED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
			LED_RED_PIN);
}

static void leds_off(void)
{
	gpio_clear(LED_GREEN_PORT, LED_GREEN_PIN);
	gpio_clear(LED_RED_PORT, LED_RED_PIN);
}

static void green_led_on(void)
{
	gpio_set(LED_GREEN_PORT, LED_GREEN_PIN);
}

static void green_led_off(void)
{
	gpio_clear(LED_GREEN_PORT, LED_GREEN_PIN);
}

static void red_led_off(void)
{
	gpio_clear(LED_RED_PORT, LED_RED_PIN);
}

static void red_led_toggle(void)
{
	gpio_toggle(LED_RED_PORT, LED_RED_PIN);
}

static void delay_ms(int ms)
{
	for (int i = 0; i < (ms * 8000); i++) {
		__asm__("nop");
	}
}

static void system_init(void)
{
	clock_init();
	leds_init();
	stm_comm_init();

	cycle_counter_available = dwt_enable_cycle_counter();
	if (!cycle_counter_available) {
		stm_comm_send("Advertencia: contador DWT no disponible\r\n");
	}
}

static bool is_space(char c)
{
	return c == ' ' || c == '\t';
}

static const char *skip_spaces(const char *text)
{
	while (is_space(*text)) {
		text++;
	}

	return text;
}

static void print_uid(void)
{
	char uid[25];

	desig_get_unique_id_as_string(uid, sizeof(uid));
	printf("uid: %s\r\n", uid);
}

static void print_sha1(const char *text)
{
	uint32_t digest[SHA1_DIGEST_WORDS];
	uint32_t start_cycles = 0;
	uint32_t elapsed_cycles = 0;
	uint64_t elapsed_ns = 0;

	if (cycle_counter_available) {
		start_cycles = dwt_read_cycle_counter();
	}

	sha1_sw((const uint8_t *)text, strlen(text), digest);

	if (cycle_counter_available) {
		elapsed_cycles = dwt_read_cycle_counter() - start_cycles;

		/*
		 * El reloj del Cortex-M4 esta configurado a 168 MHz:
		 * tiempo_ns = ciclos * 1000 / 168.
		 */
		elapsed_ns = ((uint64_t)elapsed_cycles * 1000U) / 168U;
	}

	printf("SHA1(%s) = %08lx%08lx%08lx%08lx%08lx\r\n", text, digest[0],
	       digest[1], digest[2], digest[3], digest[4]);

	if (cycle_counter_available) {
		printf("Tiempo SHA1: %lu ciclos (%lu.%03lu us)\r\n",
		       (unsigned long)elapsed_cycles,
		       (unsigned long)(elapsed_ns / 1000U),
		       (unsigned long)(elapsed_ns % 1000U));
	} else {
		stm_comm_send("Tiempo SHA1: no disponible\r\n");
	}
}

static void test_sha1_sw(void)
{
	static const uint32_t expected[SHA1_DIGEST_WORDS] = {
		0xa9993e36,
		0x4706816a,
		0xba3e2571,
		0x7850c26c,
		0x9cd0d89d,
	};
	uint32_t digest[SHA1_DIGEST_WORDS];

	sha1_sw((const uint8_t *)"abc", 3, digest);
	if (memcmp(digest, expected, sizeof(expected)) == 0) {
		stm_comm_send("SHA1 software test: OK\r\n");
	} else {
		printf("SHA1 software test: ERROR (%08lx%08lx%08lx%08lx%08lx)\r\n",
		       digest[0], digest[1], digest[2], digest[3], digest[4]);
	}
}

static app_state_t handle_normal_command(const char *command)
{
	command = skip_spaces(command);

	if (strcmp(command, "uid") == 0) {
		print_uid();
		return APP_NORMAL;
	}

	if (strncmp(command, "hash", 4) == 0 &&
	    (command[4] == '\0' || is_space(command[4]))) {
		const char *text = skip_spaces(command + 4);

		if (*text == '\0') {
			stm_comm_send("uso: hash <texto>\r\n");
			return APP_NORMAL;
		}

		print_sha1(text);
		return APP_NORMAL;
	}

	if (strcmp(command, "hash_test") == 0) {
		test_sha1_sw();
		return APP_NORMAL;
	}

	if (strcmp(command, "bloqueo") == 0) {
		leds_off();
		stm_comm_send("bloqueado\r\n");
		return APP_BLOCKED;
	}

	if (strcmp(command, "ping") == 0) {
		stm_comm_send("pong\r\n");
		return APP_NORMAL;
	}

	if (strcmp(command, "green") == 0) {
		gpio_toggle(LED_GREEN_PORT, LED_GREEN_PIN);
		stm_comm_send("green toggled\r\n");
		return APP_NORMAL;
	}

	if (strcmp(command, "red") == 0) {
		gpio_toggle(LED_RED_PORT, LED_RED_PIN);
		stm_comm_send("red toggled\r\n");
		return APP_NORMAL;
	}

	stm_comm_send("comando desconocido\r\n");
	return APP_NORMAL;
}

static app_state_t handle_blocked_command(const char *command)
{
	if (strcmp(command, UNLOCK_PASSWORD) == 0) {
		stm_comm_send("password ok\r\n");
		leds_off();
		return APP_UNLOCKED_BLINK;
	}

	stm_comm_send("password incorrecta\r\n");
	return APP_BLOCKED;
}

int main(void)
{
	char line[64];
	app_state_t state = APP_NORMAL;
	int block_print_ticks = 0;
	int led_blink_ticks = 0;
	int green_blinks = 0;
	bool green_is_on = false;

	system_init();

	stm_comm_send("STM32F429 communication ready\r\n");
	stm_comm_send("Commands: uid, hash <texto>, hash_test, bloqueo\r\n");

	while (true) {
		if (stm_comm_read_line(line, sizeof(line)) > 0) {
			if (state == APP_BLOCKED) {
				state = handle_blocked_command(line);
				block_print_ticks = 0;
				led_blink_ticks = 0;
				green_blinks = 0;
				green_is_on = false;
			} else {
				state = handle_normal_command(line);
				if (state == APP_BLOCKED) {
					block_print_ticks = 0;
					led_blink_ticks = 0;
				}
			}
		}

		if (state == APP_BLOCKED) {
			if (block_print_ticks >= BLOCK_PRINT_PERIOD_MS) {
				stm_comm_send("bloqueo\r\n");
				block_print_ticks = 0;
			}

			if (led_blink_ticks >= LED_BLINK_PERIOD_MS) {
				red_led_toggle();
				led_blink_ticks = 0;
			}
		}

		if (state == APP_UNLOCKED_BLINK) {
			if (led_blink_ticks >= LED_BLINK_PERIOD_MS) {
				if (green_is_on) {
					green_led_off();
					green_is_on = false;

					if (green_blinks >= UNLOCK_GREEN_BLINKS) {
						state = APP_NORMAL;
					}
				} else {
					red_led_off();
					green_led_on();
					green_is_on = true;
					green_blinks++;
				}

				led_blink_ticks = 0;
			}
		}

		delay_ms(1);
		block_print_ticks++;
		led_blink_ticks++;
	}
}
