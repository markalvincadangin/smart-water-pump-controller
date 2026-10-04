package com.smartflow.viewmodel

import com.smartflow.domain.CommandState
import com.smartflow.domain.ConnectionState
import com.smartflow.domain.OperatingMode
import com.smartflow.domain.PumpState
import com.smartflow.domain.ShadowReported
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class DashboardStateReducerTest {

    @Test
    fun emergencyStopTakesPrecedenceOverGenericFault() {
        val state = derivePumpState(
            connection = ConnectionState.CONNECTED,
            reported = ShadowReported(
                isError = true,
                emergencyStopLatched = true,
                lastFaultCode = "E_STOP"
            )
        )

        assertEquals(PumpState.Interlocked, state)
    }

    @Test
    fun maximumRuntimeUsesCanonicalFaultCode() {
        val reported = ShadowReported(
            isError = true,
            lastFaultCode = "MAX_RUNTIME"
        )

        assertTrue(isFaultActive(reported))
        assertTrue(isMaxRuntimeFault(reported))
        assertEquals("Maximum runtime protection", faultTitle(reported.lastFaultCode))
    }

    @Test
    fun legacyOverflowFlagStillIdentifiesMaximumRuntime() {
        val reported = ShadowReported(
            isError = true,
            isOverflowError = true,
            lastFaultCode = ""
        )

        assertTrue(isMaxRuntimeFault(reported))
    }

    @Test
    fun manualPowerCommandCompletesOnlyWhenReportedStateMatches() {
        val command = DashboardCommand.ManualPower(on = true)
        val reported = ShadowReported(
            runMode = "MANUAL_ON",
            isRunning = true
        )

        assertTrue(
            isCommandSatisfied(
                command = command,
                currentMode = OperatingMode.MANUAL,
                reported = reported
            )
        )
    }

    @Test
    fun countdownStartRequiresReportedActiveCountdown() {
        val command = DashboardCommand.CountdownStart
        val inactive = ShadowReported(runMode = "COUNTDOWN", countdownRemainingSec = 0)
        val active = inactive.copy(countdownRemainingSec = 90)

        assertTrue(
            !isCommandSatisfied(command, OperatingMode.COUNTDOWN, inactive)
        )
        assertTrue(
            isCommandSatisfied(command, OperatingMode.COUNTDOWN, active)
        )
    }

    @Test
    fun countdownStopCompletesOnlyAfterReportedCountdownStops() {
        val command = DashboardCommand.CountdownStop
        val active = ShadowReported(runMode = "COUNTDOWN", countdownRemainingSec = 30)
        val stopped = active.copy(countdownRemainingSec = 0)

        assertTrue(
            !isCommandSatisfied(command, OperatingMode.COUNTDOWN, active)
        )
        assertTrue(
            isCommandSatisfied(command, OperatingMode.COUNTDOWN, stopped)
        )
    }

    @Test
    fun interlockBlocksPendingNormalCommand() {
        val state = deriveCommandState(
            command = DashboardCommand.ManualPower(on = true),
            connection = ConnectionState.CONNECTED,
            currentMode = OperatingMode.MANUAL,
            reported = ShadowReported(
                isError = true,
                emergencyStopLatched = true,
                lastFaultCode = "E_STOP"
            )
        )

        assertEquals(CommandState.InterlockBlocked, state)
    }

    @Test
    fun faultRejectsPendingNormalCommandWhenNoInterlockIsLatched() {
        val state = deriveCommandState(
            command = DashboardCommand.ManualPower(on = true),
            connection = ConnectionState.CONNECTED,
            currentMode = OperatingMode.MANUAL,
            reported = ShadowReported(
                isError = true,
                lastFaultCode = "DRY_RUN",
                lastFaultMessage = "Dry-run lockout"
            )
        )

        assertEquals(
            CommandState.Rejected("Dry-run lockout"),
            state
        )
    }

    @Test
    fun disconnectedPendingCommandIsOfflineBlocked() {
        val state = deriveCommandState(
            command = DashboardCommand.ManualPower(on = true),
            connection = ConnectionState.DISCONNECTED,
            currentMode = OperatingMode.MANUAL,
            reported = ShadowReported()
        )

        assertEquals(CommandState.OfflineBlocked, state)
    }

    @Test
    fun emergencyStopCommandCompletesWhenLatchIsReported() {
        val state = deriveCommandState(
            command = DashboardCommand.EmergencyStop,
            connection = ConnectionState.CONNECTED,
            currentMode = OperatingMode.MANUAL,
            reported = ShadowReported(
                isError = true,
                emergencyStopLatched = true,
                lastFaultCode = "E_STOP"
            )
        )

        assertEquals(CommandState.Completed, state)
    }
}
