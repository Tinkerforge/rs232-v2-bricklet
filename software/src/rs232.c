/* rs232-v2-bricklet
 * Copyright (C) 2018 Ishraq Ibne Ashraf <ishraq@tinkerforge.com>
 *
 * rs232.c: RS232 V2 handling
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

#include "rs232.h"

#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/ringbuffer.h"
#include "bricklib2/logging/logging.h"

#include "xmc_scu.h"
#include "xmc_usic.h"
#include "xmc_uart.h"

#include "communication.h"
#include "configs/config.h"

#define rs232_rx_irq_handler  IRQ_Hdlr_11
#define rs232_tx_irq_handler  IRQ_Hdlr_12
#define rs232_rxa_irq_handler IRQ_Hdlr_13

/*
 * Set const pointers to RX ringbuffer variables.
 * With this the compiler can properly optimize the access!
 */

// RX part of buffer always starts from 0.
uint8_t  *const rs232_rb_rx_buffer = &(rs232.buffer[0]);
uint16_t *const rs232_rb_rx_end = &(rs232.rb_rx.end);
uint16_t *const rs232_rb_rx_start = &(rs232.rb_rx.start);
uint16_t *const rs232_rb_rx_size = &(rs232.rb_rx.size);

RS232_t rs232;

void __attribute__((optimize("-O3"))) __attribute__ ((section (".ram_code"))) rs232_rx_irq_handler() {
	NVIC_DisableIRQ((IRQn_Type)RS232_IRQ_RX);
	NVIC_DisableIRQ((IRQn_Type)RS232_IRQ_RXA);

	while(!XMC_USIC_CH_RXFIFO_IsEmpty(RS232_USIC)) {
		uint8_t rx_byte = RS232_USIC->OUTR;
		uint16_t rb_rx_used = (*rs232_rb_rx_end < *rs232_rb_rx_start) ? \
		                      (*rs232_rb_rx_size + *rs232_rb_rx_end - *rs232_rb_rx_start) : \
		                      (*rs232_rb_rx_end - *rs232_rb_rx_start);

		if (rs232.flowcontrol != RS232_V2_FLOWCONTROL_OFF) {
			// Flow control enabled.
			if(rs232.flowcontrol == RS232_V2_FLOWCONTROL_SOFTWARE) {
				// Software flow control.
				if (rx_byte == FC_SW_XON) {
					rs232.fc_sw_state_tx = FC_SW_STATE_TX_OK;

					// We don't treat XON/XOFF control byte as data.
					continue;
				}
				else if (rx_byte == FC_SW_XOFF) {
					rs232.fc_sw_state_tx = FC_SW_STATE_TX_WAIT;

					// We don't treat XON/XOFF control byte as data.
					continue;
				}

				if((rs232.buffer_size_rx - rb_rx_used) <= FC_RB_RX_LIMIT) {
					// We can't RX more data.

					// TX XOFF from rs232_tick().
					rs232.fc_sw_tx_xoff = true;
					rs232.fc_sw_state_rx = FC_SW_STATE_RX_WAIT;
				}
			}
			else if(rs232.flowcontrol == RS232_V2_FLOWCONTROL_HARDWARE) {
				// Hardware flow control.
				if((rs232.buffer_size_rx - rb_rx_used) <= FC_RB_RX_LIMIT) {
					// We can't RX more data.

					/*
					 * XMC_GPIO_SetOutputHigh(RS232_RTS_PIN);
					 * |
					 * | ---> RS232 logic 0.
					 *
					 * XMC_GPIO_SetOutputLow(RS232_RTS_PIN);
					 * |
					 * | ---> RS232 logic 1.
					 */

					// De-assert RTS pin.
					XMC_GPIO_SetOutputHigh(RS232_RTS_PIN);
				}
			}
		}

		/*
		 * Instead of ringbuffer_add() we add the byte to the buffer by hand.
		 * We need to save the low watermark calculation overhead.
		 */
		uint16_t new_end = *rs232_rb_rx_end + 1;

		if(new_end >= *rs232_rb_rx_size) {
			new_end = 0;
		}

		if(new_end == *rs232_rb_rx_start) {
			rs232._error_count_overrun++;

			// In the case of an overrun we read the byte and throw it away.
			volatile uint8_t __attribute__((unused)) _  = RS232_USIC->OUTR;
		}
		else {
			//rs232_rb_rx_buffer[*rs232_rb_rx_end] = RS232_USIC->OUTR;
			rs232_rb_rx_buffer[*rs232_rb_rx_end] = rx_byte;
			*rs232_rb_rx_end = new_end;
		}
	}

	NVIC_EnableIRQ((IRQn_Type)RS232_IRQ_RX);
	NVIC_EnableIRQ((IRQn_Type)RS232_IRQ_RXA);
}

void __attribute__((optimize("-O3"))) __attribute__ ((section (".ram_code"))) rs232_tx_irq_handler() {
	while(!XMC_USIC_CH_TXFIFO_IsFull(RS232_USIC)) {
		// TX FIFO is not full, more data can be loaded on the FIFO from the ringbuffer.
		uint8_t data;

		if(rs232.flowcontrol != RS232_V2_FLOWCONTROL_OFF) {
			if(rs232.flowcontrol == RS232_V2_FLOWCONTROL_SOFTWARE) {
				if(rs232.fc_sw_state_tx == FC_SW_STATE_TX_WAIT) {
					XMC_USIC_CH_TXFIFO_DisableEvent(RS232_USIC,
					                                XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);

					return;
				}
			}
			else if(rs232.flowcontrol == RS232_V2_FLOWCONTROL_HARDWARE) {
				/*
				 * XMC_GPIO_GetInput(RS232_CTS_PIN);
				 * |
				 * | ---> Return = 0 = RS232 logic 1.
				 * | ---> Return = 1 = RS232 logic 0.
				 */
				if(XMC_GPIO_GetInput(RS232_CTS_PIN) == 1) {
					XMC_USIC_CH_TXFIFO_DisableEvent(RS232_USIC,
					                                XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);

					return;
				}
			}
		}

		if(!ringbuffer_get(&rs232.rb_tx, &data)) {
			// No more data to TX from the ringbuffer, disable TX interrupt.
			XMC_USIC_CH_TXFIFO_DisableEvent(RS232_USIC,
			                                XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);

			return;
		}

		RS232_USIC->IN[0] = data;
	}
}

void __attribute__((optimize("-O3"))) rs232_rxa_irq_handler() {
	/*
	 * We get alternate RX interrupt if there is a parity error.
	 * In this case we still read the byte and give it to the user.
	 */
	rs232_rx_irq_handler();

	rs232._error_count_parity++;
}



static void rs232_init_hardware() {
	logd("[+] RS232-V2: rs232_init_hardware()\n\r");

	// RTS/CTS pin configuration.
	XMC_GPIO_CONFIG_t rts_pin_config = {
		.mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level = XMC_GPIO_OUTPUT_LEVEL_LOW
	};

	XMC_GPIO_CONFIG_t cts_pin_config = {
		.mode = XMC_GPIO_MODE_INPUT_PULL_DOWN,
	};

	// RX pin configuration.
	const XMC_GPIO_CONFIG_t rx_pin_config = {
		.mode = XMC_GPIO_MODE_INPUT_PULL_UP,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	// Configure TX pin high during configuration,
	// otherwise there can be glitches on TX line if
	// configuration changes.
	XMC_GPIO_CONFIG_t tx_pin_config_high = {
		.mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH
	};

	// Configure  pins.
	XMC_GPIO_Init(RS232_TX_PIN, &tx_pin_config_high);
	XMC_GPIO_Init(RS232_RTS_PIN, &rts_pin_config);
	XMC_GPIO_Init(RS232_CTS_PIN, &cts_pin_config);
	XMC_GPIO_Init(RS232_RX_PIN, &rx_pin_config);

	// Initialize USIC channel in UART mode.

	// USIC channel configuration.
	XMC_UART_CH_CONFIG_t cfg_uart_ch;

	cfg_uart_ch.baudrate = rs232.baudrate;
	cfg_uart_ch.stop_bits = rs232.stopbits;
	cfg_uart_ch.data_bits = rs232.wordlength;
	cfg_uart_ch.oversampling = rs232.oversampling;
	cfg_uart_ch.frame_length = rs232.wordlength;

	switch(rs232.parity) {
		case RS232_V2_PARITY_NONE:
			cfg_uart_ch.parity_mode = XMC_USIC_CH_PARITY_MODE_NONE;
			break;

		case RS232_V2_PARITY_ODD:
			cfg_uart_ch.parity_mode = XMC_USIC_CH_PARITY_MODE_ODD;
			break;

		case RS232_V2_PARITY_EVEN:
			cfg_uart_ch.parity_mode = XMC_USIC_CH_PARITY_MODE_EVEN;
			break;
	}

	XMC_UART_CH_Init(RS232_USIC, &cfg_uart_ch);

	// Set input source path.
	XMC_UART_CH_SetInputSource(RS232_USIC, RS232_RX_INPUT, RS232_RX_SOURCE);

	// Configure TX FIFO.
	XMC_USIC_CH_TXFIFO_Configure(RS232_USIC, 32, XMC_USIC_CH_FIFO_SIZE_32WORDS, 16);

	// Configure RX FIFO.
	XMC_USIC_CH_RXFIFO_Configure(RS232_USIC, 0, XMC_USIC_CH_FIFO_SIZE_32WORDS, 16);

	// UART protocol events.

	// Set service request for RX FIFO receive interrupt.
	XMC_USIC_CH_RXFIFO_SetInterruptNodePointer(
		RS232_USIC,
		XMC_USIC_CH_RXFIFO_INTERRUPT_NODE_POINTER_STANDARD,
		RS232_SERVICE_REQUEST_RX
	);

	XMC_USIC_CH_RXFIFO_SetInterruptNodePointer(
		RS232_USIC,
		XMC_USIC_CH_RXFIFO_INTERRUPT_NODE_POINTER_ALTERNATE,
		RS232_SERVICE_REQUEST_RX
	);

	// Set service request for TX FIFO transmit interrupt.
	XMC_USIC_CH_TXFIFO_SetInterruptNodePointer(
		RS232_USIC,
		XMC_USIC_CH_TXFIFO_INTERRUPT_NODE_POINTER_STANDARD,
		RS232_SERVICE_REQUEST_TX
	);

	XMC_USIC_CH_SetInterruptNodePointer(
		RS232_USIC,
		XMC_USIC_CH_INTERRUPT_NODE_POINTER_ALTERNATE_RECEIVE,
		RS232_SERVICE_REQUEST_RXA
	);

	// Set priority and enable NVIC node for TX interrupt.
	NVIC_SetPriority((IRQn_Type)RS232_IRQ_TX, RS232_IRQ_TX_PRIORITY);
	XMC_SCU_SetInterruptControl(RS232_IRQ_TX, RS232_IRQCTRL_TX);
	NVIC_EnableIRQ((IRQn_Type)RS232_IRQ_TX);

	// Set priority and enable NVIC node for RX interrupt.
	NVIC_SetPriority((IRQn_Type)RS232_IRQ_RX, RS232_IRQ_RX_PRIORITY);
	XMC_SCU_SetInterruptControl(RS232_IRQ_RX, RS232_IRQCTRL_RX);
	NVIC_EnableIRQ((IRQn_Type)RS232_IRQ_RX);

	// Set priority and enable NVIC node for RXA interrupt.
	NVIC_SetPriority((IRQn_Type)RS232_IRQ_RXA, RS232_IRQ_RXA_PRIORITY);
	XMC_SCU_SetInterruptControl(RS232_IRQ_RXA, RS232_IRQCTRL_RXA);
	NVIC_EnableIRQ((IRQn_Type)RS232_IRQ_RXA);

	// Restart UART: Stop.
	while(XMC_UART_CH_Stop(RS232_USIC) != XMC_UART_CH_STATUS_OK) {
		;
	}

	// Restart UART: Start.
	XMC_UART_CH_Start(RS232_USIC);


	// TX pin configuration.
	const XMC_GPIO_CONFIG_t tx_pin_config = {
		.mode = RS232_TX_PIN_AF,
		.output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH
	};
	XMC_GPIO_Init(RS232_TX_PIN, &tx_pin_config);

	XMC_USIC_CH_RXFIFO_EnableEvent(
		RS232_USIC,
		XMC_USIC_CH_RXFIFO_EVENT_CONF_STANDARD | XMC_USIC_CH_RXFIFO_EVENT_CONF_ALTERNATE
	);

	XMC_USIC_CH_EnableEvent(RS232_USIC, XMC_USIC_CH_EVENT_ALTERNATIVE_RECEIVE);
}

static void rs232_init_buffer(void) {
	logd("[+] RS232-V2: rs232_init_buffer()\n\r");

	// Initialize RS232 buffer.
	memset(rs232.buffer, 0, RS232_BUFFER_SIZE);

	// Ringbuffer initialization.
	ringbuffer_init(&rs232.rb_rx, rs232.buffer_size_rx, &rs232.buffer[0]);
	ringbuffer_init(&rs232.rb_tx,
	                rs232.buffer_size_tx,
	                &rs232.buffer[rs232.buffer_size_rx]);
}

void rs232_apply_configuration() {
	logd("[+] RS232-V2: rs232_apply_configuration()\n\r");

	// Turning off all events.
	XMC_USIC_CH_RXFIFO_DisableEvent(
		RS232_USIC,
		XMC_USIC_CH_RXFIFO_EVENT_CONF_STANDARD | XMC_USIC_CH_RXFIFO_EVENT_CONF_ALTERNATE
	);
	XMC_USIC_CH_TXFIFO_DisableEvent(RS232_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
	XMC_USIC_CH_DisableEvent(RS232_USIC, XMC_USIC_CH_EVENT_ALTERNATIVE_RECEIVE);

	// Now we can configure the buffer and the hardware.
	rs232_init_buffer();
	rs232_init_hardware();
}

void reset_read_stream_status() {
	rs232.read_stream_status.in_progress = false;
	rs232.read_stream_status.stream_sent = 0;
	rs232.read_stream_status.stream_chunk_offset = 0;
	rs232.read_stream_status.stream_total_length = 0;
	rs232.frame_readable_cb_already_sent = false;
}

void rs232_init() {
	logd("[+] RS232-V2: rs232_init()\n\r");

	// Initialize RS232 default configuration.
	rs232.baudrate = 115200;
	rs232.parity = RS232_V2_PARITY_NONE;
	rs232.stopbits = (uint8_t)RS232_V2_STOPBITS_1;
	rs232.wordlength = (uint8_t)RS232_V2_WORDLENGTH_8;
	rs232.flowcontrol = RS232_V2_FLOWCONTROL_OFF;
	rs232.oversampling = 16;

	rs232._error_count_parity = 0;
	rs232._error_count_overrun = 0;
	rs232.error_count_parity = 0;
	rs232.error_count_overrun = 0;
	rs232.do_error_count_callback = false;

	rs232.read_callback_enabled = false;
	rs232.frame_readable_cb_frame_size = 0;

	rs232.buffer_size_rx = RS232_BUFFER_SIZE / 2;
	rs232.buffer_size_tx = RS232_BUFFER_SIZE / 2;

	rs232.fc_sw_tx_xoff = false;
	rs232.fc_sw_state_rx = FC_SW_STATE_RX_OK;
	rs232.fc_sw_state_tx = FC_SW_STATE_TX_OK;

	reset_read_stream_status();
	rs232_apply_configuration();
}

void rs232_tick() {
	/*
	 * We try to read the RX buffer in every tick:
	 *
	 * 1. We may have data in the buffer (just by coincidence).
	 *    In this case we can save the interrupt call overhead.
	 *
	 * 2. The interrupt is only triggered if more then 16 bytes
	 *    are in the RX buffer, so we always read the end of
	 *    a big message or messages <16 bytes here.
	 */
	rs232_rx_irq_handler();

	// Manage flow control.
	if(rs232.flowcontrol == RS232_V2_FLOWCONTROL_SOFTWARE) {
		if(rs232.fc_sw_tx_xoff) {
			rs232.fc_sw_tx_xoff = false;

			// TX XOFF.
			ringbuffer_add(&rs232.rb_tx, FC_SW_XOFF);
			XMC_USIC_CH_TXFIFO_EnableEvent(RS232_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
			XMC_USIC_CH_TriggerServiceRequest(RS232_USIC, RS232_SERVICE_REQUEST_TX);
		}

		if(rs232.fc_sw_state_tx == FC_SW_STATE_TX_OK) {
			/*
			 * Initiate TX.
			 * There can be data in the TX buffer waiting to be TXed.
			 */
			XMC_USIC_CH_TXFIFO_EnableEvent(RS232_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
			XMC_USIC_CH_TriggerServiceRequest(RS232_USIC, RS232_SERVICE_REQUEST_TX);
		}

		if((rs232.fc_sw_state_rx == FC_SW_STATE_RX_WAIT) &&
		   ((rs232.buffer_size_rx - ringbuffer_get_used(&rs232.rb_rx)) > FC_RB_RX_LIMIT)) {
		      // We can RX data.
		      rs232.fc_sw_state_rx = FC_SW_STATE_RX_OK;
		}
	}
	else if(rs232.flowcontrol == RS232_V2_FLOWCONTROL_HARDWARE) {
		if((rs232.buffer_size_rx - ringbuffer_get_used(&rs232.rb_rx)) > FC_RB_RX_LIMIT) {
			// We can RX data.

			/*
			 * XMC_GPIO_SetOutputHigh(RS232_RTS_PIN);
			 * |
			 * | ---> RS232 logic 0.
			 *
			 * XMC_GPIO_SetOutputLow(RS232_RTS_PIN);
			 * |
			 * | ---> RS232 logic 1.
			 */

			// Assert RTS pin.
			XMC_GPIO_SetOutputLow(RS232_RTS_PIN);
		}

		/*
		 * XMC_GPIO_GetInput(RS232_CTS_PIN);
		 * |
		 * | ---> Return = 0 = RS232 logic 1.
		 * | ---> Return = 1 = RS232 logic 0.
		 */
		if(XMC_GPIO_GetInput(RS232_CTS_PIN) == 0) {
			/*
			 * Initiate TX.
			 * There can be data in the TX buffer waiting to be TXed.
			 */
			XMC_USIC_CH_TXFIFO_EnableEvent(RS232_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
			XMC_USIC_CH_TriggerServiceRequest(RS232_USIC, RS232_SERVICE_REQUEST_TX);
		}
	}

	// Manage error count.
	if((rs232.error_count_parity != rs232._error_count_parity) ||
		 (rs232.error_count_overrun != rs232._error_count_overrun)) {
				rs232.error_count_parity = rs232._error_count_parity;
				rs232.error_count_overrun = rs232._error_count_overrun;
				rs232.do_error_count_callback = true;
	}
}
