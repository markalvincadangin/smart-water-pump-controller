/**
 * @file hardware.h
 * @brief Hardware pin definitions and physical layout constants.
 *
 * This decouples the physical hardware wiring from the software
 * business logic.
 */
#pragma once

// ---- Pump Relay ----
#define PIN_RELAY             4

// ---- RS-485 Module (UART2) ----
#define PIN_RS485_TX          17
#define PIN_RS485_RX          25
#define PIN_RS485_DE_RE       5   // LOW = RX, HIGH = TX
#define RS485_UART_BAUD       115200

// ---- Physical Controls ----
#define PIN_RESET_BUTTON 32

// ---- I2C / Display / Other Hardware (Future) ----
// (Add new pin definitions here as hardware evolves)
