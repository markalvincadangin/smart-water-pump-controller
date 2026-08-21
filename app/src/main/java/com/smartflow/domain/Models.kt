package com.smartflow.domain

import java.time.Instant

enum class OperatingMode {
    AUTO,
    MANUAL,
    COUNTDOWN
}

enum class ConnectionState {
    CONNECTED,
    DISCONNECTED,
    CONNECTING
}

sealed interface CommandState {
    data object Ready : CommandState
    data object Pending : CommandState
    data object Accepted : CommandState
    data object Completed : CommandState
    data class Rejected(val reason: String) : CommandState
    data object TimedOut : CommandState
    data object OfflineBlocked : CommandState
    data object InterlockBlocked : CommandState
}

sealed interface PumpState {
    data object Idle : PumpState
    data object Starting : PumpState
    data object Running : PumpState
    data object Stopping : PumpState
    data object Error : PumpState
    data object Offline : PumpState
    data object Interlocked : PumpState
    data object Maintenance : PumpState
}

enum class DataFreshness { Live, Delayed, Stale, Unavailable }
enum class AlarmPriority { Critical, Warning, Advisory }
enum class AlarmCondition { Active, Cleared }
enum class AlarmAcknowledgement { Acknowledged, Unacknowledged }
enum class ControlAuthority { Local, Remote }
enum class SensorAvailability { Available, Unavailable, Bypassed }

sealed interface TelemetryValue<out T> {
    data class Available<T>(
        val value: T,
        val timestamp: Instant
    ) : TelemetryValue<T>

    data class Stale<T>(
        val lastKnownValue: T,
        val timestamp: Instant
    ) : TelemetryValue<T>

    data object Unavailable : TelemetryValue<Nothing>
}

data class DeviceShadow(
    val desired: ShadowDesired = ShadowDesired(),
    val reported: ShadowReported = ShadowReported()
)

data class ShadowDesired(
    val mode: String = OperatingMode.AUTO.name,
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
    val waterLevel: TelemetryValue<Int> = TelemetryValue.Unavailable,
    val flowRate: TelemetryValue<Float> = TelemetryValue.Unavailable,
    val distanceCm: TelemetryValue<Float> = TelemetryValue.Unavailable
)

data class DeviceConfig(
    val lowLevelThreshold: Int = 20,
    val dryRunThresholdLmin: Float = 1.0f,
    val maxOverflowTimeoutMins: Int = 30
)

data class DashboardUiState(
    val pumpState: PumpState = PumpState.Offline,
    val operatingMode: OperatingMode = OperatingMode.AUTO,
    val desiredMode: OperatingMode = OperatingMode.AUTO,
    val waterLevel: TelemetryValue<Int> = TelemetryValue.Unavailable,
    val flowRate: TelemetryValue<Float> = TelemetryValue.Unavailable,
    val connectionStatus: ConnectionState = ConnectionState.CONNECTING,
    val config: DeviceConfig = DeviceConfig(),
    val countdownRemainingSec: Int = 0,
    val countdownDurationMin: Int = 0,
    val levelSensorAvailability: SensorAvailability = SensorAvailability.Available,
    val flowSensorAvailability: SensorAvailability = SensorAvailability.Available,
    val controlAuthority: ControlAuthority = ControlAuthority.Remote,
    val dataFreshness: DataFreshness = DataFreshness.Unavailable,
    val lastFaultMessage: String = "",
    val events: List<DeviceEvent> = emptyList(),
    val commandState: CommandState = CommandState.Ready
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
