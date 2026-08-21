/**
 * @file rs485_slave.h
 * @brief RS-485 slave transceiver management for sensor nodes.
 */
#pragma once

#include "../config/config.h"

/**
 * @brief Initialize DE/RE GPIO and put transceiver in RX mode.
 */
void rs485_slave_init();

/**
 * @brief Service UART RX; responds to REQ frames.
 */
void rs485_slave_poll();

