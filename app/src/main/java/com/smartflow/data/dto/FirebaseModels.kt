package com.smartflow.data.dto

import com.google.firebase.database.PropertyName
import com.smartflow.domain.*

data class ShadowDesiredDto(
    @get:PropertyName("mode") @set:PropertyName("mode") var mode: String = ControlMode.AUTO.name,
    @get:PropertyName("manual_desired") @set:PropertyName("manual_desired") var manualDesired: Boolean = false,
    @get:PropertyName("countdown_start") @set:PropertyName("countdown_start") var countdownStart: Boolean = false,
    @get:PropertyName("countdown_duration_min") @set:PropertyName("countdown_duration_min") var countdownDurationMin: Int = 0,
    @get:PropertyName("emergency_stop") @set:PropertyName("emergency_stop") var emergencyStop: Boolean = false,
    @get:PropertyName("reset_stop") @set:PropertyName("reset_stop") var resetStop: Boolean = false,
    @get:PropertyName("clear_error") @set:PropertyName("clear_error") var clearError: Boolean = false,
    @get:PropertyName("bypass_level_sensor") @set:PropertyName("bypass_level_sensor") var bypassLevelSensor: Boolean = true,
    @get:PropertyName("bypass_flow_sensor") @set:PropertyName("bypass_flow_sensor") var bypassFlowSensor: Boolean = true,
    @get:PropertyName("reboot_device") @set:PropertyName("reboot_device") var rebootDevice: Boolean = false
) {
    fun toDomain() = ShadowDesired(
        mode = mode,
        manualDesired = manualDesired,
        countdownStart = countdownStart,
        countdownDurationMin = countdownDurationMin,
        emergencyStop = emergencyStop,
        resetStop = resetStop,
        clearError = clearError,
        bypassLevelSensor = bypassLevelSensor,
        bypassFlowSensor = bypassFlowSensor,
        rebootDevice = rebootDevice
    )
}

fun ShadowDesired.toDto() = ShadowDesiredDto(
    mode = mode,
    manualDesired = manualDesired,
    countdownStart = countdownStart,
    countdownDurationMin = countdownDurationMin,
    emergencyStop = emergencyStop,
    resetStop = resetStop,
    clearError = clearError,
    bypassLevelSensor = bypassLevelSensor,
    bypassFlowSensor = bypassFlowSensor,
    rebootDevice = rebootDevice
)

data class ShadowReportedDto(
    @get:PropertyName("run_mode") @set:PropertyName("run_mode") var runMode: String = "",
    @get:PropertyName("is_running") @set:PropertyName("is_running") var isRunning: Boolean = false,
    @get:PropertyName("countdown_remaining_sec") @set:PropertyName("countdown_remaining_sec") var countdownRemainingSec: Int = 0,
    @get:PropertyName("is_error") @set:PropertyName("is_error") var isError: Boolean = false,
    @get:PropertyName("is_overflow_error") @set:PropertyName("is_overflow_error") var isOverflowError: Boolean = false,
    @get:PropertyName("emergency_stop_latched") @set:PropertyName("emergency_stop_latched") var emergencyStopLatched: Boolean = false,
    @get:PropertyName("last_fault_message") @set:PropertyName("last_fault_message") var lastFaultMessage: String = ""
) {
    fun toDomain() = ShadowReported(
        runMode = runMode,
        isRunning = isRunning,
        countdownRemainingSec = countdownRemainingSec,
        isError = isError,
        isOverflowError = isOverflowError,
        emergencyStopLatched = emergencyStopLatched,
        lastFaultMessage = lastFaultMessage
    )
}

data class DeviceShadowDto(
    val desired: ShadowDesiredDto = ShadowDesiredDto(),
    val reported: ShadowReportedDto = ShadowReportedDto()
) {
    fun toDomain() = DeviceShadow(
        desired = desired.toDomain(),
        reported = reported.toDomain()
    )
}

data class TelemetryDto(
    @get:PropertyName("water_level_percent") @set:PropertyName("water_level_percent") var waterLevel: Int = 0,
    @get:PropertyName("flow_rate_lpm") @set:PropertyName("flow_rate_lpm") var flowRate: Float = 0f,
    @get:PropertyName("ultrasonic_last_good_cm") @set:PropertyName("ultrasonic_last_good_cm") var distanceCm: Float = 0f
) {
    fun toDomain() = Telemetry(
        waterLevel = waterLevel,
        flowRate = flowRate,
        distanceCm = distanceCm
    )
}

data class DeviceEventDto(
    // Notice we do NOT use PropertyName for id because Firebase keys are the IDs.
    // The previous implementation mapped timestamp, severity, etc.
    @get:PropertyName("timestamp") @set:PropertyName("timestamp") var timestamp: Long = 0L,
    @get:PropertyName("severity") @set:PropertyName("severity") var severity: String = "",
    @get:PropertyName("category") @set:PropertyName("category") var category: String = "",
    @get:PropertyName("code") @set:PropertyName("code") var code: String = "",
    @get:PropertyName("message") @set:PropertyName("message") var message: String = ""
) {
    fun toDomain(id: String): DeviceEvent {
        // Look up the definition in our central registry
        val definition = com.smartflow.domain.EventRegistry.getEventDefinition(code)

        val finalCategory = definition?.category ?: category
        val finalTitle = definition?.title ?: category
        val finalLogMessage = definition?.logMessage ?: message.replace(Regex("^\\[.*?\\]\\s*"), "")
        val finalNotificationMessage = definition?.notificationMessage ?: finalLogMessage

        return DeviceEvent(
            id = id,
            timestamp = timestamp,
            severity = severity,
            category = finalCategory,
            code = code,
            title = finalTitle,
            logMessage = finalLogMessage,
            notificationMessage = finalNotificationMessage,
            rawMessage = message
        )
    }
}

data class DeviceStatusDto(
    @get:PropertyName("lifecycle") @set:PropertyName("lifecycle") var lifecycle: String = "",
    @get:PropertyName("uptimeSeconds") @set:PropertyName("uptimeSeconds") var uptimeSeconds: Long = 0L
)
