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
		case FID_SET_BUFFER_CONFIG: return set_buffer_config(message);
		case FID_GET_BUFFER_CONFIG: return get_buffer_config(message, response);
		case FID_GET_BUFFER_STATUS: return get_buffer_status(message, response);
		case FID_GET_ERROR_COUNT: return get_error_count(message, response);
		case FID_SET_FRAME_READABLE_CALLBACK_CONFIGURATION: return set_frame_readable_callback_configuration(message);
		case FID_GET_FRAME_READABLE_CALLBACK_CONFIGURATION: return get_frame_readable_callback_configuration(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}

BootloaderHandleMessageResponse write_low_level(const WriteLowLevel *data, WriteLowLevel_Response *response) {
	uint8_t written = 0;
	response->header.length = sizeof(WriteLowLevel_Response);

	if((data->message_length - data->message_chunk_offset) >= sizeof(data->message_chunk_data)) {
		// Whole chunk with data.
		for(written = 0; written < sizeof(data->message_chunk_data); written++) {
			if(!ringbuffer_add(&rs232.rb_tx, data->message_chunk_data[written])) {
				break;
			}
		}
	}
	else {
		// Partial chunk with data.
		for(written = 0; written < (data->message_length - data->message_chunk_offset); written++) {
			if(!ringbuffer_add(&rs232.rb_tx, data->message_chunk_data[written])) {
				break;
			}
		}
	}

	if(written != 0) {
		XMC_USIC_CH_TXFIFO_EnableEvent(RS232_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
		XMC_USIC_CH_TriggerServiceRequest(RS232_USIC, RS232_SERVICE_REQUEST_TX);
	}

	response->message_chunk_written = written;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse read_low_level(const ReadLowLevel *data, ReadLowLevel_Response *response) {
	uint16_t rb_available = 0;
	response->message_length = 0;
	response->message_chunk_offset = 0;
	response->header.length = sizeof(ReadLowLevel_Response);

	// This function operates only when read callback is disabled.
	if(rs232.read_callback_enabled) {
		return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
	}

	rb_available = ringbuffer_get_used(&rs232.rb_rx);

	if(rb_available == 0) {
		// There are no data available at the moment in the RX buffer.
		reset_read_stream_status();

		return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
	}

	if(!rs232.read_stream_status.in_progress) {
		// Start of new stream.
		rs232.read_stream_status.in_progress = true;

		if(data->length >= rb_available) {
			/*
			 * Requested total data is more than or equal to currently available data.
			 * So create a stream to transfer all of the currently available data.
			 */
			rs232.read_stream_status.stream_total_length = rb_available;
		}
		else {
			rs232.read_stream_status.stream_total_length = data->length;
		}

		rs232.read_stream_status.stream_chunk_offset = 0;

		response->message_chunk_offset = rs232.read_stream_status.stream_chunk_offset;
		response->message_length = rs232.read_stream_status.stream_total_length;

		if(response->message_length <= sizeof(response->message_chunk_data)) {
			// Available data fits in a single chunk.
			for(uint8_t i = 0; i < response->message_length; i++) {
				ringbuffer_get(&rs232.rb_rx, (uint8_t *)&response->message_chunk_data[i]);
			}

			reset_read_stream_status();
		}
		else {
			// Requested data requires more than one chunk.
			for(uint8_t i = 0; i < sizeof(response->message_chunk_data); i++) {
				ringbuffer_get(&rs232.rb_rx, (uint8_t *)&response->message_chunk_data[i]);
			}

			rs232.read_stream_status.stream_sent += sizeof(response->message_chunk_data);
		}
	}
	else {
		// Handle a stream which is already in progress.
		response->message_chunk_offset = rs232.read_stream_status.stream_sent;
		response->message_length = rs232.read_stream_status.stream_total_length;

		if((rs232.read_stream_status.stream_total_length - rs232.read_stream_status.stream_sent) >= \
			sizeof(response->message_chunk_data)) {
				for(uint8_t i = 0; i < sizeof(response->message_chunk_data); i++) {
					ringbuffer_get(&rs232.rb_rx, (uint8_t *)&response->message_chunk_data[i]);
				}

				rs232.read_stream_status.stream_sent += sizeof(response->message_chunk_data);
		}
		else {
			for(uint8_t i = 0; i < (rs232.read_stream_status.stream_total_length - rs232.read_stream_status.stream_sent); i++) {
				ringbuffer_get(&rs232.rb_rx, (uint8_t *)&response->message_chunk_data[i]);
			}

			rs232.read_stream_status.stream_sent += \
				(rs232.read_stream_status.stream_total_length - rs232.read_stream_status.stream_sent);
		}

		if(rs232.read_stream_status.stream_total_length - rs232.read_stream_status.stream_sent == 0) {
			// Last chunk of the stream.
			reset_read_stream_status();
		}
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse enable_read_callback(const EnableReadCallback *data) {
	logd("[+] RS232-V2: enable_read_callback()\n\r");

	rs232.read_callback_enabled = true;
	rs232.frame_readable_cb_frame_size = 0;

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

BootloaderHandleMessageResponse get_error_count(const GetErrorCount *data, GetErrorCount_Response *response) {
	logd("[+] RS232-V2: get_error_count()\n\r");

	response->header.length = sizeof(GetErrorCount_Response);
	response->error_count_overrun = rs232.error_count_overrun;
	response->error_count_parity = rs232.error_count_parity;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_frame_readable_callback_configuration(const SetFrameReadableCallbackConfiguration *data) {
	if(data->frame_size > 0) {
		rs232.read_callback_enabled = false;
	}
	rs232.frame_readable_cb_frame_size = data->frame_size;
	rs232.frame_readable_cb_already_sent = false;
	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_frame_readable_callback_configuration(const GetFrameReadableCallbackConfiguration *data, GetFrameReadableCallbackConfiguration_Response *response) {
	response->header.length = sizeof(GetFrameReadableCallbackConfiguration_Response);
	response->frame_size = rs232.frame_readable_cb_frame_size;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

bool handle_read_low_level_callback(void) {
	static uint16_t used = 0;
	static ReadLowLevel_Callback cb;
	static bool is_buffered = false;
	static uint16_t count_rb_read = 0;

	if(!is_buffered) {
		if(!rs232.read_callback_enabled) {
			is_buffered = false;

			return false;
		}

		used = ringbuffer_get_used(&rs232.rb_rx);

		if(used > 0 || rs232.read_stream_status.in_progress) {
			tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(ReadLowLevel_Callback), FID_CALLBACK_READ_LOW_LEVEL);
			cb.message_length = 0;
			cb.message_chunk_offset = 0;

			if(!rs232.read_stream_status.in_progress) {
				// Start of new stream.
				reset_read_stream_status();

				cb.message_length = used;
				cb.message_chunk_offset = 0;

				rs232.read_stream_status.in_progress = true;
				rs232.read_stream_status.stream_total_length = used;

				if(cb.message_length <= sizeof(cb.message_chunk_data)) {
					// Available data fits in a single chunk.
					reset_read_stream_status();
					count_rb_read = cb.message_length;
				}
				else {
					// Available data requires more than one chunk.
					count_rb_read = sizeof(cb.message_chunk_data);
				}
			}
			else {
				// Handle a stream which is already in progress.
				cb.message_chunk_offset = rs232.read_stream_status.stream_sent;
				cb.message_length = rs232.read_stream_status.stream_total_length;

				if((rs232.read_stream_status.stream_total_length - rs232.read_stream_status.stream_sent) >= \
					sizeof(cb.message_chunk_data)) {
						count_rb_read = sizeof(cb.message_chunk_data);
				}
				else {
					count_rb_read = rs232.read_stream_status.stream_total_length - rs232.read_stream_status.stream_sent;
				}

				if(rs232.read_stream_status.stream_total_length == rs232.read_stream_status.stream_sent + count_rb_read) {
					// Last chunk of the stream.
					reset_read_stream_status();
				}
			}

			if(count_rb_read > 0) {
				for(uint8_t i = 0; i < count_rb_read; i++) {
					ringbuffer_get(&rs232.rb_rx, (uint8_t *)&cb.message_chunk_data[i]);
				}

				rs232.read_stream_status.stream_sent += count_rb_read;
			}
		}
		else {
			reset_read_stream_status();
			is_buffered = false;

			return false;
		}
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(ReadLowLevel_Callback));
		is_buffered = false;

		return true;
	}
	else {
		is_buffered = true;
	}

	return false;
}

bool handle_error_count_callback(void) {
	static bool is_buffered = false;
	static ErrorCount_Callback cb;

	if(!is_buffered) {
		if (!rs232.do_error_count_callback) {
			return false;
		}

		tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(ErrorCount_Callback), FID_CALLBACK_ERROR_COUNT);
		cb.error_count_overrun = rs232.error_count_overrun;
		cb.error_count_parity = rs232.error_count_parity;
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(ErrorCount_Callback));
		is_buffered = false;
		rs232.do_error_count_callback = false;

		return true;
	}
	else {
		is_buffered = true;
	}

	return false;
}

bool handle_frame_readable_callback(void) {
	static bool is_buffered = false;
	static FrameReadable_Callback cb;
	static uint16_t used = 0;

	if(!is_buffered) {
		if(rs232.frame_readable_cb_frame_size == 0) {
			return false;
		}

		if(rs232.frame_readable_cb_already_sent) {
			return false;
		}

		used = ringbuffer_get_used(&rs232.rb_rx);
		if (used < rs232.frame_readable_cb_frame_size) {
			return false;
		}
		rs232.frame_readable_cb_already_sent = true;

		tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(FrameReadable_Callback), FID_CALLBACK_FRAME_READABLE);
		cb.frame_count = used / rs232.frame_readable_cb_frame_size;
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(FrameReadable_Callback));
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
