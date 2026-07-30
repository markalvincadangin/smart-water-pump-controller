package com.smartflow.domain

import com.google.firebase.database.PropertyName

enum class ControlMode {
    AUTO,
    MANUAL,
    COUNTDOWN
}

enum class ConnectionState {
    CONNECTED,
    DISCONNECTED,
    CONNECTING
}
data class DeviceShadow(
    val desired: ShadowDesired = ShadowDesired(),
    val reported: ShadowReported = ShadowReported()
)

data class ShadowDesired(
    @get:PropertyName("mode") @set:PropertyName("mode") var mode: String = ControlMode.AUTO.name,
    @get:PropertyName("manual_desired") @set:PropertyName("manual_desired") var manualDesired: Boolean = false,
    @get:PropertyName("countdown_start") @set:PropertyName("countdown_start") var countdownStart: Boolean = false,
    @get:PropertyName("countdown_duration_min") @set:PropertyName("countdown_duration_min") var countdownDurationMin: Int = 0,
    @get:PropertyName("emergency_stop") @set:PropertyName("emergency_stop") var emergencyStop: Boolean = false,
    @get:PropertyName("reset_stop") @set:PropertyName("reset_stop") var resetStop: Boolean = false,
    @get:PropertyName("clear_error") @set:PropertyName("clear_error") var clearError: Boolean = false,
    @get:PropertyName("bypass_level_sensor") @set:PropertyName("bypass_level_sensor") var bypassLevelSensor: Boolean = true,
    @get:PropertyName("bypass_flow_sensor") @set:PropertyName("bypass_flow_sensor") var bypassFlowSensor: Boolean = true
)

data class ShadowReported(
    @get:PropertyName("run_mode") @set:PropertyName("run_mode") var runMode: String = "",
    @get:PropertyName("is_running") @set:PropertyName("is_running") var isRunning: Boolean = false,
    @get:PropertyName("countdown_remaining_sec") @set:PropertyName("countdown_remaining_sec") var countdownRemainingSec: Int = 0,
    @get:PropertyName("is_error") @set:PropertyName("is_error") var isError: Boolean = false,
    @get:PropertyName("is_overflow_error") @set:PropertyName("is_overflow_error") var isOverflowError: Boolean = false,
    @get:PropertyName("emergency_stop_latched") @set:PropertyName("emergency_stop_latched") var emergencyStopLatched: Boolean = false,
    @get:PropertyName("last_fault_message") @set:PropertyName("last_fault_message") var lastFaultMessage: String = ""
)

data class Telemetry(
    @get:PropertyName("water_level_percent") @set:PropertyName("water_level_percent") var waterLevel: Int = 0,
    @get:PropertyName("flow_rate_lpm") @set:PropertyName("flow_rate_lpm") var flowRate: Float = 0f,
    @get:PropertyName("ultrasonic_last_good_cm") @set:PropertyName("ultrasonic_last_good_cm") var distanceCm: Float = 0f
)

data class DeviceConfig(
    val lowLevelThreshold: Int = 20,
    val dryRunThresholdLmin: Float = 1.0f,
    val maxOverflowTimeoutMins: Int = 30
)

data class DashboardUiState(
    val isPumpRunning: Boolean = false,
    val mode: ControlMode = ControlMode.AUTO,
    val lockoutActive: Boolean = false,
    val waterLevelPct: Int = 0,
    val flowRateLpm: Float = 0f,
    val connectionStatus: ConnectionState = ConnectionState.DISCONNECTED,
    val config: DeviceConfig = DeviceConfig(),
    val countdownRemainingSec: Int = 0,
    val bypassLevelSensor: Boolean = true,
    val bypassFlowSensor: Boolean = true,
    val isManualDesired: Boolean = false,
    val isCountdownStartDesired: Boolean = false,
    val lastFaultMessage: String = "",
    val events: List<DeviceEvent> = emptyList()
)

data class DeviceEvent(
    @get:PropertyName("timestamp") @set:PropertyName("timestamp") var timestamp: Long = 0L,
    @get:PropertyName("severity") @set:PropertyName("severity") var severity: String = "",
    @get:PropertyName("category") @set:PropertyName("category") var category: String = "",
    @get:PropertyName("code") @set:PropertyName("code") var code: String = "",
    @get:PropertyName("message") @set:PropertyName("message") var message: String = ""
)
