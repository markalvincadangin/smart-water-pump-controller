package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.data.repository.DeviceRepository
import com.smartflow.domain.ControlMode
import com.smartflow.domain.DashboardUiState
import com.smartflow.domain.DeviceConfig
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
        repository.connectionFlow
    ) { telemetry, shadow, config, connection ->
        val currentMode = when (shadow.reported.runMode) {
            "MANUAL", "MANUAL_ON", "MANUAL_OFF", "MANUAL_COOLDOWN" -> ControlMode.MANUAL
            "COUNTDOWN" -> ControlMode.COUNTDOWN
            else -> ControlMode.AUTO
        }
        
        DashboardUiState(
            isPumpRunning = shadow.reported.isRunning,
            mode = currentMode,
            lockoutActive = shadow.reported.isError || shadow.reported.isOverflowError,
            waterLevelPct = telemetry.waterLevel,
            flowRateLpm = telemetry.flowRate,
            connectionStatus = connection,
            config = config,
            countdownRemainingSec = shadow.reported.countdownRemainingSec,
            bypassLevelSensor = shadow.desired.bypassLevelSensor,
            bypassFlowSensor = shadow.desired.bypassFlowSensor
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
            mode = uiState.value.mode.name,
            manualDesired = on
        )
        repository.updateDesiredState(desired)
    }

    fun setControlMode(mode: ControlMode) {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = mode.name,
            manualDesired = uiState.value.isPumpRunning,
            countdownStart = false // Reset countdown start when switching modes
        )
        repository.updateDesiredState(desired)
    }

    fun triggerEmergencyStop() {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = ControlMode.MANUAL.name,
            emergencyStop = true
        )
        repository.updateDesiredState(desired)
    }
    
    fun startCountdown(durationMin: Int) {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = ControlMode.COUNTDOWN.name,
            countdownStart = true,
            countdownDurationMin = durationMin
        )
        repository.updateDesiredState(desired)
    }
    
    fun clearErrors() {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = uiState.value.mode.name,
            clearError = true,
            resetStop = true
        )
        repository.updateDesiredState(desired)
    }

    fun updateConfig(config: DeviceConfig) {
        repository.updateConfig(config)
    }

    fun updateBypass(bypassLevel: Boolean, bypassFlow: Boolean) {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = uiState.value.mode.name,
            bypassLevelSensor = bypassLevel,
            bypassFlowSensor = bypassFlow
        )
        repository.updateDesiredState(desired)
    }
}
