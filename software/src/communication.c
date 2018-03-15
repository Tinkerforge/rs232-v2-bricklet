/* rs232-v2-bricklet
 * Copyright (C) 2018 Ishraq Ibne Ashraf <ishraq@tinkerforge.com>
 *
 * communication.c: TFP protocol message handling
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

#include "communication.h"

#include "bricklib2/utility/communication_callback.h"
#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/logging/logging.h"

#include "xmc_usic.h"
#include "xmc_uart.h"

#include "rs232.h"

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
		case FID_WRITE_LOW_LEVEL: return write_low_level(message, response);
		case FID_READ_LOW_LEVEL: return read_low_level(message, response);
		case FID_ENABLE_READ_CALLBACK: return enable_read_callback(message);
		case FID_DISABLE_READ_CALLBACK: return disable_read_callback(message);
		case FID_IS_READ_CALLBACK_ENABLED: return is_read_callback_enabled(message, response);
		case FID_SET_CONFIGURATION: return set_configuration(message);
		case FID_GET_CONFIGURATION: return get_configuration(message, response);
		case FID_SET_BREAK_CONDITION: return set_break_condition(message);
		case FID_SET_BUFFER_CONFIG: return set_buffer_config(message);
		case FID_GET_BUFFER_CONFIG: return get_buffer_config(message, response);
		case FID_GET_BUFFER_STATUS: return get_buffer_status(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}

BootloaderHandleMessageResponse write_low_level(const WriteLowLevel *data, WriteLowLevel_Response *response) {
	response->header.length = sizeof(WriteLowLevel_Response);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse read_low_level(const ReadLowLevel *data, ReadLowLevel_Response *response) {
	response->header.length = sizeof(ReadLowLevel_Response);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse enable_read_callback(const EnableReadCallback *data) {
	logd("[+] RS232-V2: enable_read_callback()\n\r");

	rs232.read_callback_enabled = true;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse disable_read_callback(const DisableReadCallback *data) {
	logd("[+] RS232-V2: disable_read_callback()\n\r");

	rs232.read_callback_enabled = false;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse is_read_callback_enabled(const IsReadCallbackEnabled *data, IsReadCallbackEnabled_Response *response) {
	logd("[+] RS232-V2: is_read_callback_enabled()\n\r");

	response->header.length = sizeof(IsReadCallbackEnabled_Response);
	response->enabled = rs232.read_callback_enabled;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_configuration(const SetConfiguration *data) {
	logd("[+] RS232-V2: set_configuration()\n\r");

	if (data->baudrate < CONFIG_BAUDRATE_MIN || data->baudrate > CONFIG_BAUDRATE_MAX) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if (data->parity > CONFIG_PARITY_MAX) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if (data->stopbits < CONFIG_STOPBITS_MIN || data->stopbits > CONFIG_STOPBITS_MAX) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if (data->wordlength < CONFIG_WORDLENGTH_MIN || data->wordlength > CONFIG_WORDLENGTH_MAX) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if (data->flowcontrol > CONFIG_FLOWCONTROL_MAX) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	rs232.baudrate = data->baudrate;
	rs232.parity = data->parity;
	rs232.stopbits = data->stopbits;
	rs232.wordlength = data->wordlength;
	rs232.flowcontrol = data->flowcontrol;

	rs232_apply_configuration();

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_configuration(const GetConfiguration *data, GetConfiguration_Response *response) {
	logd("[+] RS232-V2: get_configuration()\n\r");

	response->header.length = sizeof(GetConfiguration_Response);
	response->baudrate = rs232.baudrate;
	response->parity = (uint8_t)rs232.parity;
	response->stopbits = rs232.stopbits;
	response->wordlength = rs232.wordlength;
	response->flowcontrol = (uint8_t)rs232.flowcontrol;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_break_condition(const SetBreakCondition *data) {
	logd("[+] RS232-V2: set_break_condition()\n\r");

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_buffer_config(const SetBufferConfig *data) {
	logd("[+] RS232-V2: set_buffer_config()\n\r");

	if((data->receive_buffer_size < 1024) ||
	   (data->send_buffer_size < 1024) ||
	   ((data->receive_buffer_size + data->send_buffer_size) != RS232_BUFFER_SIZE)) {

		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	rs232.buffer_size_rx = data->receive_buffer_size;
	rs232.buffer_size_tx = data->send_buffer_size;

	rs232_apply_configuration();

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_buffer_config(const GetBufferConfig *data, GetBufferConfig_Response *response) {
	logd("[+] RS232-V2: get_buffer_config()\n\r");

	response->header.length = sizeof(GetBufferConfig_Response);
	response->receive_buffer_size = rs232.buffer_size_rx;
	response->send_buffer_size = rs232.buffer_size_tx;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_buffer_status(const GetBufferStatus *data, GetBufferStatus_Response *response) {
	logd("[+] RS232-V2: get_buffer_status()\n\r");

	response->header.length = sizeof(GetBufferStatus_Response);
	response->send_buffer_used = ringbuffer_get_used(&rs232.rb_rx);
	response->receive_buffer_used = ringbuffer_get_used(&rs232.rb_tx);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

bool handle_read_low_level_callback(void) {
	static bool is_buffered = false;
	static ReadLowLevel_Callback cb;

	if(!is_buffered) {
		tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(ReadLowLevel_Callback), FID_CALLBACK_READ_LOW_LEVEL);
		// TODO: Implement ReadLowLevel callback handling

		return false;
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(ReadLowLevel_Callback));
		is_buffered = false;
		return true;
	} else {
		is_buffered = true;
	}

	return false;
}

bool handle_error_callback(void) {
	static bool is_buffered = false;
	static Error_Callback cb;

	if(!is_buffered) {
		tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(Error_Callback), FID_CALLBACK_ERROR);
		// TODO: Implement Error callback handling

		return false;
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(Error_Callback));
		is_buffered = false;
		return true;
	} else {
		is_buffered = true;
	}

	return false;
}

void communication_tick(void) {
	communication_callback_tick();
}

void communication_init(void) {
	communication_callback_init();
}
