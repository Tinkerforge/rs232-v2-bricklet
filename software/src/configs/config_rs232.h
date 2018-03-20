/* rs232-v2-bricklet
 * Copyright (C) 2018 Ishraq Ibne Ashraf <ishraq@tinkerforge.com>
 *
 * config_rs232.h: RS232 V2 specific configurations
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

#ifndef CONFIG_RS232_H
#define CONFIG_RS232_H

#include "xmc_common.h"

#define RS232_USIC_CHANNEL        USIC1_CH1
#define RS232_USIC                XMC_UART1_CH1

#define RS232_RX_PIN              P2_13
#define RS232_RX_INPUT            XMC_USIC_CH_INPUT_DX0
#define RS232_RX_SOURCE           0b011 // DX0D.

#define RS232_TX_PIN              P2_12
#define RS232_TX_PIN_AF           (XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT7 | P2_12_AF_U1C1_DOUT0)

// RTS/CTS pins.
#define RS232_RTS_PIN             P2_11
#define RS232_CTS_PIN             P2_10

// Interrupts.
#define RS232_SERVICE_REQUEST_RX  2  // RX.
#define RS232_SERVICE_REQUEST_TX  3  // TX.
#define RS232_SERVICE_REQUEST_RXA 4  // Alternate RX (parity error).

#define RS232_IRQ_RX              11
#define RS232_IRQ_RX_PRIORITY     0
#define RS232_IRQCTRL_RX          XMC_SCU_IRQCTRL_USIC1_SR2_IRQ11

#define RS232_IRQ_TX              12
#define RS232_IRQ_TX_PRIORITY     1
#define RS232_IRQCTRL_TX          XMC_SCU_IRQCTRL_USIC1_SR3_IRQ12

#define RS232_IRQ_RXA             13
#define RS232_IRQ_RXA_PRIORITY    0
#define RS232_IRQCTRL_RXA         XMC_SCU_IRQCTRL_USIC1_SR4_IRQ13

#endif
