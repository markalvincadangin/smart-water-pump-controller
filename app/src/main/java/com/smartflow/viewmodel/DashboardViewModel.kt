package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.data.repository.DeviceRepository
import com.smartflow.domain.ConnectionState
import com.smartflow.domain.OperatingMode
import com.smartflow.domain.DashboardUiState
import com.smartflow.domain.DeviceConfig
import com.smartflow.domain.PumpState
import com.smartflow.domain.CommandState
import com.smartflow.domain.SensorAvailability
import com.smartflow.domain.ControlAuthority
import com.smartflow.domain.DataFreshness
import com.smartflow.domain.TelemetryValue
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class DashboardViewModel(
    private val repository: DeviceRepository
) : ViewModel() {

    val uiState: StateFlow<DashboardUiState> = combine(
        repository.telemetryFlow,
        repository.shadowFlow,
        repository.configFlow,
        repository.connectionFlow,
        repository.eventsFlow
    ) { telemetry, shadow, config, connection, events ->
        val desiredMode = when (shadow.desired.mode) {
            "MANUAL" -> OperatingMode.MANUAL
            "COUNTDOWN" -> OperatingMode.COUNTDOWN
            else -> OperatingMode.AUTO
        }
        
        val currentMode = when (shadow.reported.runMode) {
            "MANUAL", "MANUAL_ON", "MANUAL_OFF", "MANUAL_COOLDOWN" -> OperatingMode.MANUAL
            "COUNTDOWN" -> OperatingMode.COUNTDOWN
            "AUTO", "SMART", "ECO" -> OperatingMode.AUTO
            "IDLE", "ERROR" -> desiredMode
            else -> desiredMode
        }
        
        val lockoutActive = shadow.reported.isError || shadow.reported.isOverflowError || shadow.reported.emergencyStopLatched
        
        val pumpState = when {
            connection == ConnectionState.DISCONNECTED -> PumpState.Offline
            shadow.reported.isError || shadow.reported.isOverflowError -> PumpState.Error
            shadow.reported.emergencyStopLatched -> PumpState.Interlocked
            shadow.reported.isRunning -> PumpState.Running
            else -> PumpState.Idle
        }
        
        val isSyncing = (desiredMode != currentMode) || (desiredMode == OperatingMode.MANUAL && shadow.desired.manualDesired != shadow.reported.isRunning && !lockoutActive)
        
        val commandState = if (connection == ConnectionState.DISCONNECTED && isSyncing) {
            CommandState.OfflineBlocked
        } else if (lockoutActive && isSyncing) {
            CommandState.Rejected(shadow.reported.lastFaultMessage.ifEmpty { "System error active" })
        } else if (isSyncing) {
            CommandState.Pending
        } else {
            CommandState.Ready
        }

        fun <T> handleTelemetryValue(tv: TelemetryValue<T>): TelemetryValue<T> {
            return if (connection == ConnectionState.DISCONNECTED && tv is TelemetryValue.Available) {
                TelemetryValue.Stale(tv.value, tv.timestamp)
            } else {
                tv
            }
        }

        DashboardUiState(
            pumpState = pumpState,
            operatingMode = currentMode,
            desiredMode = desiredMode,
            waterLevel = handleTelemetryValue(telemetry.waterLevel),
            flowRate = handleTelemetryValue(telemetry.flowRate),
            connectionStatus = connection,
            config = config,
            countdownRemainingSec = shadow.reported.countdownRemainingSec,
            countdownDurationMin = shadow.desired.countdownDurationMin,
            levelSensorAvailability = if (shadow.desired.bypassLevelSensor) SensorAvailability.Bypassed else SensorAvailability.Available,
            flowSensorAvailability = if (shadow.desired.bypassFlowSensor) SensorAvailability.Bypassed else SensorAvailability.Available,
            controlAuthority = ControlAuthority.Remote,
            dataFreshness = if (connection == ConnectionState.CONNECTED) DataFreshness.Live else DataFreshness.Stale,
            lastFaultMessage = shadow.reported.lastFaultMessage,
            events = events,
            commandState = commandState
        )
    }.stateIn(
        scope = viewModelScope,
        started = SharingStarted.WhileSubscribed(5000),
        initialValue = DashboardUiState()
    )

    init {
        viewModelScope.launch {
            repository.initializeAuth()
        }
    }

    fun setPumpPower(on: Boolean) {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = uiState.value.operatingMode.name,
            manualDesired = on,
            clearError = false,
            resetStop = false
        )
        repository.updateDesiredState(desired)
    }

    fun setControlMode(mode: OperatingMode) {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = mode.name,
            manualDesired = uiState.value.pumpState is PumpState.Running,
            countdownStart = false, // Reset countdown start when switching modes
            clearError = false,
            resetStop = false
        )
        repository.updateDesiredState(desired)
    }

    fun triggerEmergencyStop() {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = OperatingMode.MANUAL.name,
            emergencyStop = true
        )
        repository.updateDesiredState(desired)
    }
    
    fun startCountdown(durationMin: Int) {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = OperatingMode.COUNTDOWN.name,
            countdownStart = true,
            countdownDurationMin = durationMin,
            clearError = false,
            resetStop = false
        )
        repository.updateDesiredState(desired)
    }

    fun stopCountdown() {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = OperatingMode.COUNTDOWN.name,
            countdownStart = false,
            clearError = false,
            resetStop = false
        )
        repository.updateDesiredState(desired)
    }
    
    fun clearErrors() {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = uiState.value.operatingMode.name,
            clearError = true,
            resetStop = true,
            emergencyStop = false
        )
        repository.updateDesiredState(desired)
    }

    fun updateConfig(config: DeviceConfig) {
        repository.updateConfig(config)
    }

    fun updateBypass(bypassLevel: Boolean, bypassFlow: Boolean) {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = uiState.value.operatingMode.name,
            bypassLevelSensor = bypassLevel,
            bypassFlowSensor = bypassFlow
        )
        repository.updateDesiredState(desired)
    }

    fun rebootDevice() {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            rebootDevice = true
        )
        repository.updateDesiredState(desired)
    }
}
