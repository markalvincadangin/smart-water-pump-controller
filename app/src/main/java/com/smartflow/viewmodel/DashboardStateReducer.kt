package com.smartflow.viewmodel

import com.smartflow.domain.CommandState
import com.smartflow.domain.ConnectionState
import com.smartflow.domain.OperatingMode
import com.smartflow.domain.PumpState
import com.smartflow.domain.ShadowReported

/**
 * Pure dashboard state/command helpers.
 *
 * These functions deliberately use only observable reported state and the
 * Android-side pending command intent. A successful RTDB write is not treated
 * as proof that the physical pump accepted or completed the command.
 */
internal sealed interface DashboardCommand {
    data class ManualPower(val on: Boolean) : DashboardCommand
    data class ModeChange(val mode: OperatingMode) : DashboardCommand
    data object CountdownStart : DashboardCommand
    data object CountdownStop : DashboardCommand
    data object EmergencyStop : DashboardCommand
    data object ClearErrors : DashboardCommand
}

internal fun mapReportedMode(runMode: String, fallback: OperatingMode): OperatingMode =
    when (runMode) {
        "MANUAL", "MANUAL_ON", "MANUAL_OFF", "MANUAL_COOLDOWN" -> OperatingMode.MANUAL
        "COUNTDOWN" -> OperatingMode.COUNTDOWN
        "AUTO", "SMART", "ECO" -> OperatingMode.AUTO
        "IDLE", "ERROR" -> fallback
        else -> fallback
    }

internal fun isFaultActive(reported: ShadowReported): Boolean =
    reported.isError ||
        reported.isOverflowError ||
        reported.lastFaultCode.isNotBlank()

internal fun isMaxRuntimeFault(reported: ShadowReported): Boolean =
    reported.lastFaultCode.equals("MAX_RUNTIME", ignoreCase = true) ||
        reported.isOverflowError

internal fun faultTitle(faultCode: String): String =
    when (faultCode.uppercase()) {
        "E_STOP" -> "Emergency stop active"
        "DRY_RUN" -> "Dry-run lockout"
        "MAX_RUNTIME" -> "Maximum runtime protection"
        "TANK_FULL", "OVERFLOW" -> "Tank-full protection"
        "LEVEL_SENSOR" -> "Level sensor fault"
        "FLOW_SENSOR" -> "Flow sensor fault"
        "COMM_LOSS" -> "Sensor communication fault"
        "STALE_LEVEL" -> "Stale level data"
        "COMMAND_REJECTED" -> "Command rejected"
        "SAFE_MODE" -> "Safe mode"
        else -> "System fault"
    }

internal fun derivePumpState(
    connection: ConnectionState,
    reported: ShadowReported
): PumpState =
    when {
        connection == ConnectionState.DISCONNECTED -> PumpState.Offline
        reported.emergencyStopLatched -> PumpState.Interlocked
        isFaultActive(reported) -> PumpState.Error
        reported.isRunning -> PumpState.Running
        else -> PumpState.Idle
    }

internal fun isCommandSatisfied(
    command: DashboardCommand,
    currentMode: OperatingMode,
    reported: ShadowReported
): Boolean =
    when (command) {
        is DashboardCommand.ManualPower ->
            currentMode == OperatingMode.MANUAL && reported.isRunning == command.on

        is DashboardCommand.ModeChange ->
            currentMode == command.mode

        DashboardCommand.CountdownStart ->
            currentMode == OperatingMode.COUNTDOWN &&
                reported.countdownRemainingSec > 0

        DashboardCommand.CountdownStop ->
            currentMode == OperatingMode.COUNTDOWN &&
                reported.countdownRemainingSec == 0

        DashboardCommand.EmergencyStop ->
            reported.emergencyStopLatched

        DashboardCommand.ClearErrors ->
            !isFaultActive(reported) && !reported.emergencyStopLatched
    }

internal fun deriveCommandState(
    command: DashboardCommand?,
    connection: ConnectionState,
    currentMode: OperatingMode,
    reported: ShadowReported
): CommandState {
    if (command == null) return CommandState.Ready

    if (connection == ConnectionState.DISCONNECTED) {
        return CommandState.OfflineBlocked
    }

    if (isCommandSatisfied(command, currentMode, reported)) {
        return CommandState.Completed
    }

    if (reported.emergencyStopLatched &&
        command !is DashboardCommand.EmergencyStop &&
        command !is DashboardCommand.ClearErrors
    ) {
        return CommandState.InterlockBlocked
    }

    if (isFaultActive(reported) &&
        command !is DashboardCommand.EmergencyStop &&
        command !is DashboardCommand.ClearErrors
    ) {
        val reason = reported.lastFaultMessage.ifEmpty {
            faultTitle(reported.lastFaultCode)
        }
        return CommandState.Rejected(reason)
    }

    return CommandState.Pending
}
