#pragma once

#include "../config/config.h"

// Initialize DE/RE GPIO and put transceiver in RX mode.
void rs485_slave_init();

// Service UART RX; responds to REQ frames.
void rs485_slave_poll();

