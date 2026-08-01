package com.smartflow.domain

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
    val mode: String = ControlMode.AUTO.name,
    val manualDesired: Boolean = false,
    val countdownStart: Boolean = false,
    val countdownDurationMin: Int = 0,
    val emergencyStop: Boolean = false,
    val resetStop: Boolean = false,
    val clearError: Boolean = false,
    val bypassLevelSensor: Boolean = true,
    val bypassFlowSensor: Boolean = true,
    val rebootDevice: Boolean = false
)

data class ShadowReported(
    val runMode: String = "",
    val isRunning: Boolean = false,
    val countdownRemainingSec: Int = 0,
    val isError: Boolean = false,
    val isOverflowError: Boolean = false,
    val emergencyStopLatched: Boolean = false,
    val lastFaultMessage: String = ""
)

data class Telemetry(
    val waterLevel: Int = 0,
    val flowRate: Float = 0f,
    val distanceCm: Float = 0f
)

data class DeviceConfig(
    val lowLevelThreshold: Int = 20,
    val dryRunThresholdLmin: Float = 1.0f,
    val maxOverflowTimeoutMins: Int = 30
)

data class DashboardUiState(
    val isPumpRunning: Boolean = false,
    val mode: ControlMode = ControlMode.AUTO,
    val desiredMode: ControlMode = ControlMode.AUTO,
    val lockoutActive: Boolean = false,
    val waterLevelPct: Int = 0,
    val flowRateLpm: Float = 0f,
    val connectionStatus: ConnectionState = ConnectionState.CONNECTING,
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
    val id: String,
    val timestamp: Long,
    val severity: String,
    val category: String,
    val code: String,
    val title: String,
    val logMessage: String,
    val notificationMessage: String,
    val rawMessage: String
)
