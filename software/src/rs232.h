/* rs232-v2-bricklet
 * Copyright (C) 2018 Ishraq Ibne Ashraf <ishraq@tinkerforge.com>
 *
 * rs232.h: RS232 V2 handling
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef RS232_H
#define RS232_H

#include <stdint.h>

#include "bricklib2/utility/ringbuffer.h"

#include "configs/config_rs232.h"

#define RS232_BUFFER_SIZE 1024*10

#define CONFIG_BAUDRATE_MIN 300
#define CONFIG_BAUDRATE_MAX 230400
#define CONFIG_PARITY_MAX 4
#define CONFIG_STOPBITS_MIN 1
#define CONFIG_STOPBITS_MAX 2
#define CONFIG_WORDLENGTH_MIN 5
#define CONFIG_WORDLENGTH_MAX 8
#define CONFIG_FLOWCONTROL_MAX 2

typedef enum {
	ERROR_OVERRUN = 1,
	ERROR_PARITY,
	ERROR_FRAMING = 4
} RS232Error_t;

typedef struct {
	uint32_t baudrate;
	int parity;
	uint8_t stopbits;
	uint8_t wordlength;
	int flowcontrol;
	uint8_t oversampling;

	uint32_t error_count_parity;
	uint32_t error_count_overrun;
	uint32_t error_count_framing;

	bool read_callback_enabled;

	Ringbuffer rb_rx;
	Ringbuffer rb_tx;
	uint8_t buffer[RS232_BUFFER_SIZE];
	uint16_t buffer_size_rx;
	uint16_t buffer_size_tx;
} RS232_t;

extern RS232_t rs232;

void rs232_init(void);
void rs232_tick(void);
void rs232_apply_configuration(void);

#endif
