/*
  Plastic Logic EPD project on MSP430

  Copyright (C) 2013 Plastic Logic Limited

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 * main.c -- main entry point for code.
 *
 * Authors: Nick Terry <nick.terry@plasticlogic.com>
 *
 */

#include <pl/platform.h>
#include <pl/hwinfo.h>
#include <stdio.h>
#include "i2c-eeprom.h"
#include "assert.h"
#include "types.h"
#include "config.h"
#include "FatFs/ff.h"
#include "msp430-i2c.h"
#include "msp430-gpio.h"
#include "msp430-sdcard.h"
#include "msp430-uart.h"
#include "plat-hbz13.h"
#include "plat-cuckoo.h"
#include "plat-hbz6.h"
#include "plat-raven.h"
#include "plat-ruddock2.h"
#include "plat-epson.h"

#define LOG_TAG "main"
#include "utils.h"

#define	ASSERT_LED              MSP430_GPIO(7,7)

/* Version of pl-mcu-epd */
static const char VERSION[] = "v006";

/* Platform instance, to be passed to other modules */
static struct platform g_plat;

int app_main(void)
{
	FATFS sdcard;
#if CONFIG_HW_INFO_EEPROM
	struct pl_hw_info pl_hw_info;
#endif
	int platform_type;

	LOG("------------------------");
	LOG("Starting pl-mcu-epd %s", VERSION);

	/* initialise GPIO interface */
	if (msp430_gpio_init(&g_plat.gpio))
		abort_msg("Failed to initialise GPIO interface");

	/* initialise common GPIOs */
	if (ruddock2_init(&g_plat.gpio))
		abort_msg("Failed to initialise GPIOs");

	/* initialise UART */
	if (uart_init(&g_plat.gpio, BR_115200, 'N', 8, 1))
		abort_msg("Failed to initialise UART");

	/* initialise MSP430 I2C master 0 */
	if (msp430_i2c_init(&g_plat.gpio, 0, &g_plat.host_i2c))
		abort_msg("Failed to initialise I2C master");

	/* initialise SD-card */
	SDCard_plat = &g_plat;
	f_chdrive(0);
	if (f_mount(0, &sdcard) != FR_OK)
		abort_msg("Failed to initialise SD card");

#if CONFIG_HW_INFO_EEPROM
	/* hardware info EEPROM */
	g_plat.hw_eeprom.i2c = &g_plat.host_i2c;
	g_plat.hw_eeprom.i2c_addr = I2C_PSU_EEPROM_ADDR;
	g_plat.hw_eeprom.type = EEPROM_24LC014;

#if 0 /* this currently fails, the Epson needs to be reset first */
	if (pl_hw_info_init(&pl_hw_info, &g_plat.hw_eeprom))
		abort_msg("Failed to read hardware info EEPROM");
#endif
#endif

#if CONFIG_PLAT_AUTO
	/* determine platform from PSU eeprom contents */
	platform_type = check_platform(&g_plat);

	if (platform_type == EPDC_S1D13524 || platform_type == EPDC_S1D13541) {
		plat_epson_init(&g_plat);
	} else {
#endif
#if CONFIG_PLAT_CUCKOO
		plat_cuckoo_init(&g_plat);
#elif CONFIG_PLAT_Z13
		plat_hbz13_init(&g_plat, CONFIG_DISPLAY_TYPE,
				CONFIG_I2C_ON_EPSON);
#elif CONFIG_PLAT_Z6 || CONFIG_PLAT_Z7
		plat_hbZn_init(&g_plat, CONFIG_DISPLAY_TYPE,
			       CONFIG_I2C_ON_EPSON);
#elif CONFIG_PLAT_RAVEN
		plat_raven_init(&g_plat);
#endif
	}

	/* Do not return from app_main */
	for (;;) {
		/* ToDo: go to sleep */
		udelay(1);
	}

#if 0 /* causes a compiler warning */
	return 0;
#endif
}

/* When something fatal has happened, this is called which prints a message on
 * stderr and flashes the ASSERT LED.
 */
void abort_now(const char *abort_msg)
{
	if (abort_msg != NULL)
		fprintf(stderr, "%s", abort_msg);

	for (;;) {
		g_plat.gpio.set(ASSERT_LED, 1);
		mdelay(500);
		g_plat.gpio.set(ASSERT_LED, 0);
		mdelay(500);
	}
}
