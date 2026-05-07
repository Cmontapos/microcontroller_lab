/*
 * Copyright (C) 2013 ARCOS-Lab Universidad de Costa Rica
 * Author: Federico Ruiz Ugalde <memeruiz@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3-plus/newlib/syscall.h>
#include "cdcacm_example.h"
#include <libopencm3-plus/newlib/devices/cdcacm.h>
#include <libopencm3-plus/newlib/devices/usart.h>
#include <stdio.h>
#include <libopencm3-plus/utils/misc.h>
#include <libopencm3-plus/stm32l0538-disco/leds.h>
#include <limits.h>
#include <stdbool.h>
#include <libopencm3/stm32/syscfg.h>

#define LED_GREEN_PIN GPIO13
#define LED_GREEN_PORT GPIOB
#define LED_RED_PIN GPIO5
#define LED_RED_PORT GPIOC


void leds_init(void) {
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
}

void system_init(void) {
  leds_init();

  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

  devoptab_list[0] = &dotab_cdcacm;
  devoptab_list[1] = &dotab_cdcacm;
  devoptab_list[2] = &dotab_cdcacm;
  cdcacm_f429_init();
  /* devoptab_list[0] = &dotab_usart; */
  /* devoptab_list[1] = &dotab_usart; */
  /* devoptab_list[2] = &dotab_usart; */
  /* usart_init(); */
}

void init_console(void) {
  setvbuf(stdin, NULL, _IONBF,
          0); // Sets stdin in unbuffered mode (normal for usart com)
  setvbuf(stdout, NULL, _IONBF,
          0); // Sets stdin in unbuffered mode (normal for usart com)

  while (lo_poll(stdin) > 0) {
    printf("Cleaning stdin\n");
    getc(stdin);
  }
  printf("Stdin cleared\n");
}

void delay_ms(int ms) {
    for (int i = 0; i < ms * 8000; i++) {
        __asm__("nop");
    }
}

int main(void)
{
  system_init();
  init_console();
  char cmd_s[50]="";
  char cmd[10]="";
  char confirm[10]="";
  int i, j;
  int c=0;
  int n_char=0;
  int running=0;
  int count=0;


  while (1){
    // Detectar Enter
    if (lo_poll(stdin) > 0) {
        char c = getc(stdin);
        putc(c, stdout);
        if (c == '\r') {
            running = 1;   // iniciar conteo
            count = 0;
        } else {
            running = 0;   // cualquier otra tecla detiene
        }
    }
    if (running) {
        printf("Count: %d\n\r", count);
        delay_ms(500);

        count++;
        if (count > 10) count = 0;
    }
  }
}
