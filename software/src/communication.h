/* rs232-v2-bricklet
 * Copyright (C) 2018 Ishraq Ibne Ashraf <ishraq@tinkerforge.com>
 *
 * communication.h: TFP protocol message handling
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

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>
#include <stdbool.h>

#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/bootloader/bootloader.h"

// Default functions
BootloaderHandleMessageResponse handle_message(const void *data, void *response);
void communication_tick(void);
void communication_init(void);

// Constants
#define RS232_V2_PARITY_NONE 0
#define RS232_V2_PARITY_ODD 1
#define RS232_V2_PARITY_EVEN 2

#define RS232_V2_STOPBITS_1 1
#define RS232_V2_STOPBITS_2 2

#define RS232_V2_WORDLENGTH_5 5
#define RS232_V2_WORDLENGTH_6 6
#define RS232_V2_WORDLENGTH_7 7
#define RS232_V2_WORDLENGTH_8 8

#define RS232_V2_FLOWCONTROL_OFF 0
#define RS232_V2_FLOWCONTROL_SOFTWARE 1
#define RS232_V2_FLOWCONTROL_HARDWARE 2

#define RS232_V2_BOOTLOADER_MODE_BOOTLOADER 0
#define RS232_V2_BOOTLOADER_MODE_FIRMWARE 1
#define RS232_V2_BOOTLOADER_MODE_BOOTLOADER_WAIT_FOR_REBOOT 2
#define RS232_V2_BOOTLOADER_MODE_FIRMWARE_WAIT_FOR_REBOOT 3
#define RS232_V2_BOOTLOADER_MODE_FIRMWARE_WAIT_FOR_ERASE_AND_REBOOT 4

#define RS232_V2_BOOTLOADER_STATUS_OK 0
#define RS232_V2_BOOTLOADER_STATUS_INVALID_MODE 1
#define RS232_V2_BOOTLOADER_STATUS_NO_CHANGE 2
#define RS232_V2_BOOTLOADER_STATUS_ENTRY_FUNCTION_NOT_PRESENT 3
#define RS232_V2_BOOTLOADER_STATUS_DEVICE_IDENTIFIER_INCORRECT 4
#define RS232_V2_BOOTLOADER_STATUS_CRC_MISMATCH 5

#define RS232_V2_STATUS_LED_CONFIG_OFF 0
#define RS232_V2_STATUS_LED_CONFIG_ON 1
#define RS232_V2_STATUS_LED_CONFIG_SHOW_HEARTBEAT 2
#define RS232_V2_STATUS_LED_CONFIG_SHOW_STATUS 3

// Function and callback IDs and structs
#define FID_WRITE_LOW_LEVEL 1
#define FID_READ_LOW_LEVEL 2
#define FID_ENABLE_READ_CALLBACK 3
#define FID_DISABLE_READ_CALLBACK 4
#define FID_IS_READ_CALLBACK_ENABLED 5
#define FID_SET_CONFIGURATION 6
#define FID_GET_CONFIGURATION 7
#define FID_SET_BUFFER_CONFIG 8
#define FID_GET_BUFFER_CONFIG 9
#define FID_GET_BUFFER_STATUS 10
#define FID_GET_ERROR_COUNT 11
#define FID_SET_AVAILABLE_CALLBACK 14

#define FID_CALLBACK_READ_LOW_LEVEL 12
#define FID_CALLBACK_ERROR_COUNT 13
#define FID_CALLBACK_AVAILABLE 15

typedef struct {
	TFPMessageHeader header;
	uint16_t message_length;
	uint16_t message_chunk_offset;
	char message_chunk_data[60];
} __attribute__((__packed__)) WriteLowLevel;

typedef struct {
	TFPMessageHeader header;
	uint8_t message_chunk_written;
} __attribute__((__packed__)) WriteLowLevel_Response;

typedef struct {
	TFPMessageHeader header;
	uint16_t length;
} __attribute__((__packed__)) ReadLowLevel;

typedef struct {
	TFPMessageHeader header;
	uint16_t message_length;
	uint16_t message_chunk_offset;
	char message_chunk_data[60];
} __attribute__((__packed__)) ReadLowLevel_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) EnableReadCallback;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) DisableReadCallback;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) IsReadCallbackEnabled;

typedef struct {
	TFPMessageHeader header;
	bool enabled;
} __attribute__((__packed__)) IsReadCallbackEnabled_Response;

typedef struct {
	TFPMessageHeader header;
	uint32_t baudrate;
	uint8_t parity;
	uint8_t stopbits;
	uint8_t wordlength;
	uint8_t flowcontrol;
} __attribute__((__packed__)) SetConfiguration;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetConfiguration;

typedef struct {
	TFPMessageHeader header;
	uint32_t baudrate;
	uint8_t parity;
	uint8_t stopbits;
	uint8_t wordlength;
	uint8_t flowcontrol;
} __attribute__((__packed__)) GetConfiguration_Response;

typedef struct {
	TFPMessageHeader header;
	uint16_t send_buffer_size;
	uint16_t receive_buffer_size;
} __attribute__((__packed__)) SetBufferConfig;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetBufferConfig;

typedef struct {
	TFPMessageHeader header;
	uint16_t send_buffer_size;
	uint16_t receive_buffer_size;
} __attribute__((__packed__)) GetBufferConfig_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetBufferStatus;

typedef struct {
	TFPMessageHeader header;
	uint16_t send_buffer_used;
	uint16_t receive_buffer_used;
} __attribute__((__packed__)) GetBufferStatus_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetErrorCount;

typedef struct {
	TFPMessageHeader header;
	uint32_t error_count_overrun;
	uint32_t error_count_parity;
} __attribute__((__packed__)) GetErrorCount_Response;

typedef struct {
	TFPMessageHeader header;
	uint16_t message_length;
	uint16_t message_chunk_offset;
	char message_chunk_data[60];
} __attribute__((__packed__)) ReadLowLevel_Callback;

typedef struct {
	TFPMessageHeader header;
	uint32_t error_count_overrun;
	uint32_t error_count_parity;
} __attribute__((__packed__)) ErrorCount_Callback;

typedef struct {
	TFPMessageHeader header;
	uint16_t frame_size;
} __attribute__((__packed__)) SetAvailableCallback;

typedef struct {
	TFPMessageHeader header;
	uint16_t count;
} __attribute__((__packed__)) Available_Callback;

// Function prototypes
BootloaderHandleMessageResponse write_low_level(const WriteLowLevel *data, WriteLowLevel_Response *response);
BootloaderHandleMessageResponse read_low_level(const ReadLowLevel *data, ReadLowLevel_Response *response);
BootloaderHandleMessageResponse enable_read_callback(const EnableReadCallback *data);
BootloaderHandleMessageResponse disable_read_callback(const DisableReadCallback *data);
BootloaderHandleMessageResponse is_read_callback_enabled(const IsReadCallbackEnabled *data, IsReadCallbackEnabled_Response *response);
BootloaderHandleMessageResponse set_configuration(const SetConfiguration *data);
BootloaderHandleMessageResponse get_configuration(const GetConfiguration *data, GetConfiguration_Response *response);
BootloaderHandleMessageResponse set_buffer_config(const SetBufferConfig *data);
BootloaderHandleMessageResponse get_buffer_config(const GetBufferConfig *data, GetBufferConfig_Response *response);
BootloaderHandleMessageResponse get_buffer_status(const GetBufferStatus *data, GetBufferStatus_Response *response);
BootloaderHandleMessageResponse get_error_count(const GetErrorCount *data, GetErrorCount_Response *response);
BootloaderHandleMessageResponse set_available_callback(const SetAvailableCallback *data);

// Callbacks
bool handle_read_low_level_callback(void);
bool handle_error_count_callback(void);
bool handle_available_callback(void);

#define COMMUNICATION_CALLBACK_TICK_WAIT_MS 1
#define COMMUNICATION_CALLBACK_HANDLER_NUM 3
#define COMMUNICATION_CALLBACK_LIST_INIT \
	handle_read_low_level_callback, \
	handle_error_count_callback, \
	handle_available_callback, \


#endif
