package com.smartflow.domain

data class DeviceConfigValidation(
    val pumpStartLevelError: String? = null,
    val pumpStopLevelError: String? = null,
    val dryRunThresholdError: String? = null,
    val maxPumpRuntimeError: String? = null
) {
    val isValid: Boolean
        get() = pumpStartLevelError == null &&
            pumpStopLevelError == null &&
            dryRunThresholdError == null &&
            maxPumpRuntimeError == null
}

object DeviceConfigValidator {
    const val PUMP_LEVEL_MIN_PCT = 0
    const val PUMP_LEVEL_MAX_PCT = 100

    const val DRY_RUN_THRESHOLD_MIN_LPM = 0.1f
    const val DRY_RUN_THRESHOLD_MAX_LPM = 10.0f

    const val MAX_PUMP_RUNTIME_MIN_MINUTES = 30
    const val MAX_PUMP_RUNTIME_MAX_MINUTES = 120

    const val COUNTDOWN_MIN_MINUTES = 1
    const val COUNTDOWN_MAX_MINUTES = 120

    fun validate(config: DeviceConfig): DeviceConfigValidation {
        val startError = when {
            config.pumpStartLevelPct !in PUMP_LEVEL_MIN_PCT..PUMP_LEVEL_MAX_PCT ->
                "Start level must be between 0% and 100%."
            else -> null
        }

        val stopError = when {
            config.pumpStopLevelPct !in PUMP_LEVEL_MIN_PCT..PUMP_LEVEL_MAX_PCT ->
                "Stop level must be between 0% and 100%."
            config.pumpStopLevelPct <= config.pumpStartLevelPct ->
                "Stop level must be greater than start level."
            else -> null
        }

        val dryRunError = when {
            config.dryRunThresholdLpm < DRY_RUN_THRESHOLD_MIN_LPM ||
                config.dryRunThresholdLpm > DRY_RUN_THRESHOLD_MAX_LPM ->
                "Dry-run threshold must be between 0.1 and 10.0 L/min."
            else -> null
        }

        val runtimeError = when {
            config.maxPumpRuntimeMin !in MAX_PUMP_RUNTIME_MIN_MINUTES..MAX_PUMP_RUNTIME_MAX_MINUTES ->
                "Maximum pump runtime must be between 30 and 120 minutes."
            else -> null
        }

        return DeviceConfigValidation(
            pumpStartLevelError = startError,
            pumpStopLevelError = stopError,
            dryRunThresholdError = dryRunError,
            maxPumpRuntimeError = runtimeError
        )
    }

    fun validateCountdownDuration(durationMin: Int): String? =
        if (durationMin in COUNTDOWN_MIN_MINUTES..COUNTDOWN_MAX_MINUTES) {
            null
        } else {
            "Countdown duration must be between 1 and 120 minutes."
        }
}
