#pragma once

#include <stdint.h>

/**
 * @brief Represents the intent received from a command source (e.g. Firebase, Bluetooth, Button).
 * This structure acts as the boundary between the command reception layer and the main execution state machine.
 */
enum class CommandType {
    NONE,
    START_MANUAL,
    START_COUNTDOWN,
    STOP,
    CLEAR_ERROR
};

struct PumpCommand {
    CommandType type;
    uint32_t durationSeconds; // Only applicable for START_COUNTDOWN, 0 otherwise

    PumpCommand() : type(CommandType::NONE), durationSeconds(0) {}
    PumpCommand(CommandType t, uint32_t d = 0) : type(t), durationSeconds(d) {}
};
