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
//#define rs232_tff_irq_handler IRQ_Hdlr_13
#define rs232_rxa_irq_handler IRQ_Hdlr_14

RS232_t rs232;

/*
void __attribute__((optimize("-O3"))) rs232_tff_irq_handler(void) {
	if((RS232_USIC.PSR_ASCMode & (XMC_UART_CH_STATUS_FLAG_TRANSMITTER_FRAME_FINISHED | XMC_UART_CH_STATUS_FLAG_TRANSFER_STATUS_BUSY)) ==
		 XMC_UART_CH_STATUS_FLAG_TRANSMITTER_FRAME_FINISHED) {
			 RS232_HALF_DUPLEX_RX_ENABLE();
	}
}
*/

void __attribute__((optimize("-O3"))) __attribute__ ((section (".ram_code"))) rs232_rx_irq_handler(void) {
	logd("[+] IRQ: RX\n\r");
	while(!XMC_USIC_CH_RXFIFO_IsEmpty(RS232_USIC)) {
		/*
		// Instead of ringbuffer_add() we add the byte to the buffer
		// by hand.
		//
		// We need to save the low watermark calculation overhead.

		uint16_t new_end = *rs232_ringbuffer_rx_end + 1;

		if(new_end >= *rs232_ringbuffer_rx_size) {
			new_end = 0;
		}

		if(new_end == *rs232_ringbuffer_rx_start) {
			rs232.error_count_overrun++;

			// In the case of an overrun we read the byte and throw it away.
			volatile uint8_t __attribute__((unused)) _  = RS232_USIC.OUTR;
		} else {
			rs232_ringbuffer_rx_buffer[*rs232_ringbuffer_rx_end] = RS232_USIC.OUTR;
			*rs232_ringbuffer_rx_end = new_end;
		}
		*/

		logd("[+] IRQ: RX=%c\n\r", RS232_USIC->OUTR);
	}
}

void __attribute__((optimize("-O3"))) rs232_rxa_irq_handler(void) {
	logd("[+] IRQ: RXA\n\r");
	// We get alternate rx interrupt if there is a parity error. In this case we
	// still read the byte and give it to the user.
	rs232.error_count_parity++;

	rs232_rx_irq_handler();
}

void __attribute__((optimize("-O3"))) __attribute__ ((section (".ram_code"))) rs232_tx_irq_handler(void) {
	logd("[+] IRQ: TX\n\r");
	RS232_USIC->IN[0] = 'D';
	RS232_USIC->IN[0] = 'E';
	RS232_USIC->IN[0] = 'A';
	RS232_USIC->IN[0] = 'D';
	RS232_USIC->IN[0] = 'B';
	RS232_USIC->IN[0] = 'E';
	RS232_USIC->IN[0] = 'E';
	RS232_USIC->IN[0] = 'F';
	XMC_USIC_CH_TXFIFO_DisableEvent(RS232_USIC,
																	XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
	logd("[+] IRQ: TX:X\n\r");
	return;
	/*
	while(!XMC_USIC_CH_TXFIFO_IsFull(RS232_USIC)) {
		// TX FIFO is not full, more data can be loaded on the FIFO from the ring buffer.
		uint8_t data;
		if(!ringbuffer_get(&rs232.ringbuffer_tx, &data)) {
			// No more data to TX from ringbuffer, disable TX interrupt.
			XMC_USIC_CH_TXFIFO_DisableEvent(RS232_USIC,
																			XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
			return;
		}

		RS232_USIC.IN[0] = data;
	}
	*/
}

static void rs232_init_hardware() {
	logd("[+] RS232-V2: rs232_init_hardware()\n\r");

	// RTS/CTS pin configuration.
	XMC_GPIO_CONFIG_t rts_pin_config = {
		.mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level = XMC_GPIO_OUTPUT_LEVEL_LOW
	};
	/*
	 * XMC_GPIO_SetOutputHigh(RS232_RTS_PIN); // -ve/0 voltage on RS232 side (RS232 logic 0).
	 * XMC_GPIO_SetOutputLow(RS232_RTS_PIN); // +ve voltage on RS232 side (RS232 logic 1).
	 */

  XMC_GPIO_CONFIG_t cts_pin_config = {
		.mode = XMC_GPIO_MODE_INPUT_PULL_DOWN,
	};
	/*
	 * Returned value 1 = -ve/0 voltage on RS232 side (RS232 logic 0).
	 * Returned value 0 = +ve voltage on RS232 side (RS232 logic 1).
	 *
	 * uint32_t XMC_GPIO_GetInput(RS232_CTS_PIN); 
	 */

	// RX pin configuration.
	const XMC_GPIO_CONFIG_t rx_pin_config = {
		.mode = XMC_GPIO_MODE_INPUT_PULL_UP,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	// TX pin configuration.
	const XMC_GPIO_CONFIG_t tx_pin_config = {
		.mode = RS232_TX_PIN_AF,
		.output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH
	};

	// Configure  pins.
	XMC_GPIO_Init(RS232_RTS_PIN, &rts_pin_config);
	XMC_GPIO_Init(RS232_CTS_PIN, &cts_pin_config);
	XMC_GPIO_Init(RS232_RX_PIN, &rx_pin_config);
	XMC_GPIO_Init(RS232_TX_PIN, &tx_pin_config);

	// Initialize USIC channel in UART mode.

	// USIC channel configuration.
	XMC_UART_CH_CONFIG_t cfg_uart_ch;

	cfg_uart_ch.baudrate = rs232.baudrate;
	cfg_uart_ch.stop_bits = rs232.stopbits;
	cfg_uart_ch.data_bits = rs232.wordlength;
	cfg_uart_ch.oversampling = rs232.oversampling;
	// TODO: Should this be wordlength?
	cfg_uart_ch.frame_length = \
		rs232.wordlength + (rs232.parity == RS232_V2_PARITY_NONE ? 0 : 1);

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

		case RS232_V2_PARITY_FORCED_PARITY_1:
			// TODO: How to set sticky parity bit ?
			cfg_uart_ch.parity_mode = XMC_USIC_CH_PARITY_MODE_NONE;
			break;

		case RS232_V2_PARITY_FORCED_PARITY_0:
			// TODO: How to set sticky parity bit ?
			cfg_uart_ch.parity_mode = XMC_USIC_CH_PARITY_MODE_NONE;
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

	XMC_USIC_CH_RXFIFO_EnableEvent(
		RS232_USIC,
		XMC_USIC_CH_RXFIFO_EVENT_CONF_STANDARD | XMC_USIC_CH_RXFIFO_EVENT_CONF_ALTERNATE
	);

	XMC_USIC_CH_EnableEvent(RS232_USIC, XMC_USIC_CH_EVENT_ALTERNATIVE_RECEIVE);

	// FIXME: Just for test. Start a TX. Remove.
	XMC_USIC_CH_TXFIFO_EnableEvent(RS232_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
	XMC_USIC_CH_TriggerServiceRequest(RS232_USIC, RS232_SERVICE_REQUEST_TX);
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

void rs232_init() {
	logd("[+] RS232-V2: rs232_init()\n\r");

	// Initialize RS232 default configuration.
	rs232.baudrate = (uint32_t)RS232_V2_BAUDRATE_115200;
	rs232.parity = RS232_V2_PARITY_NONE;
	rs232.stopbits = (uint8_t)RS232_V2_STOPBITS_1;
	rs232.wordlength = (uint8_t)RS232_V2_WORDLENGTH_8;
	rs232.flowcontrol = RS232_V2_FLOWCONTROL_OFF;
	rs232.oversampling = 16;

	rs232.error_count_parity = 0;
	rs232.error_count_overrun = 0;
	rs232.error_count_framing = 0;

	rs232.read_callback_enabled = false;

	rs232.buffer_size_rx = RS232_BUFFER_SIZE / 2;
	rs232.buffer_size_tx = RS232_BUFFER_SIZE / 2;

	rs232_apply_configuration();
}

void rs232_tick() {
  ;
}
