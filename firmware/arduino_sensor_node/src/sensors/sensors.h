#pragma once

#include "../config/config.h"

// Initialize GPIO + ISR attachment.
void sensors_init();

// Non-blocking periodic sensor service (updates cached values).
void sensors_update_nonblocking();

// Snapshot of last cached values (thread-safe-ish: copy primitives).
void sensors_get_last_values(int& levelPctOut, float& distanceCmOut, float& flowLpmOut, int& errCodeOut, uint8_t& seqOut);

