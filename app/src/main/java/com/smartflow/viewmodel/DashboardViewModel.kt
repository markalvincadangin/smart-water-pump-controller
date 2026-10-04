package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.data.repository.DeviceRepository
import com.smartflow.domain.ConnectionState
import com.smartflow.domain.OperatingMode
import com.smartflow.domain.DashboardUiState
import com.smartflow.domain.DeviceConfig
import com.smartflow.domain.DeviceConfigValidator
import com.smartflow.domain.CommandState
import com.smartflow.domain.PendingCommandType
import com.smartflow.domain.SensorAvailability
import com.smartflow.domain.ControlAuthority
import com.smartflow.domain.DataFreshness
import com.smartflow.domain.TelemetryValue
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class DashboardViewModel(
    private val repository: DeviceRepository
) : ViewModel() {

    private val pendingCommand = MutableStateFlow<DashboardCommand?>(null)
    private val commandOutcome = MutableStateFlow<CommandState?>(null)
    private var commandTimeoutJob: Job? = null

    private data class RepositoryState(
        val telemetry: com.smartflow.domain.Telemetry,
        val shadow: com.smartflow.domain.DeviceShadow,
        val config: DeviceConfig,
        val connection: ConnectionState,
        val events: List<com.smartflow.domain.DeviceEvent>
    )

    private val repositoryStateFlow = combine(
        repository.telemetryFlow,
        repository.shadowFlow,
        repository.configFlow,
        repository.connectionFlow,
        repository.eventsFlow
    ) { telemetry, shadow, config, connection, events ->
        RepositoryState(telemetry, shadow, config, connection, events)
    }

    val uiState: StateFlow<DashboardUiState> = combine(
        repositoryStateFlow,
        pendingCommand,
        commandOutcome
    ) { repoState, pending, outcome ->
        val telemetry = repoState.telemetry
        val shadow = repoState.shadow
        val config = repoState.config
        val connection = repoState.connection
        val events = repoState.events

        val desiredMode = mapDesiredMode(shadow.desired.mode)
        val currentMode = mapReportedMode(shadow.reported.runMode, desiredMode)
        val reported = shadow.reported

        fun <T> handleTelemetryValue(tv: TelemetryValue<T>): TelemetryValue<T> {
            return if (connection == ConnectionState.DISCONNECTED && tv is TelemetryValue.Available) {
                TelemetryValue.Stale(tv.value, tv.timestamp)
            } else {
                tv
            }
        }

        DashboardUiState(
            pumpState = derivePumpState(connection, reported),
            operatingMode = currentMode,
            desiredMode = desiredMode,
            waterLevel = handleTelemetryValue(telemetry.waterLevel),
            flowRate = handleTelemetryValue(telemetry.flowRate),
            connectionStatus = connection,
            config = config,
            countdownRemainingSec = reported.countdownRemainingSec,
            countdownDurationMin = shadow.desired.countdownDurationMin,
            levelSensorAvailability = if (shadow.desired.bypassLevelSensor) SensorAvailability.Bypassed else SensorAvailability.Available,
            flowSensorAvailability = if (shadow.desired.bypassFlowSensor) SensorAvailability.Bypassed else SensorAvailability.Available,
            controlAuthority = ControlAuthority.Remote,
            dataFreshness = if (connection == ConnectionState.CONNECTED) DataFreshness.Live else DataFreshness.Stale,
            lastFaultMessage = reported.lastFaultMessage,
            lastFaultCode = reported.lastFaultCode,
            events = events,
            pendingCommandType = pending?.let(::pendingCommandType),
            commandState = outcome ?: deriveCommandState(
                command = pending,
                connection = connection,
                currentMode = currentMode,
                reported = reported
            )
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
        observeCommandConfirmation()
    }

    private fun observeCommandConfirmation() {
        viewModelScope.launch {
            combine(repository.shadowFlow, repository.connectionFlow) { shadow, connection ->
                shadow to connection
            }.collect { (shadow, connection) ->
                val command = pendingCommand.value ?: return@collect
                val desiredMode = mapDesiredMode(shadow.desired.mode)
                val currentMode = mapReportedMode(shadow.reported.runMode, desiredMode)
                val reported = shadow.reported

                val commandState = deriveCommandState(
                    command = command,
                    connection = connection,
                    currentMode = currentMode,
                    reported = reported
                )

                when (commandState) {
                    CommandState.Completed -> finishCommand(CommandState.Completed)
                    is CommandState.Rejected -> finishCommand(commandState)
                    CommandState.InterlockBlocked -> finishCommand(CommandState.InterlockBlocked)
                    else -> Unit
                }
            }
        }
    }

    private fun pendingCommandType(command: DashboardCommand): PendingCommandType =
        when (command) {
            is DashboardCommand.ManualPower -> PendingCommandType.MANUAL_POWER
            is DashboardCommand.ModeChange -> PendingCommandType.MODE_CHANGE
            DashboardCommand.CountdownStart -> PendingCommandType.COUNTDOWN_START
            DashboardCommand.CountdownStop -> PendingCommandType.COUNTDOWN_STOP
            DashboardCommand.EmergencyStop -> PendingCommandType.EMERGENCY_STOP
            DashboardCommand.ClearErrors -> PendingCommandType.CLEAR_ERRORS
        }

    private fun submitCommand(
        command: DashboardCommand,
        desired: com.smartflow.domain.ShadowDesired
    ) {
        if (repository.connectionFlow.value != ConnectionState.CONNECTED) {
            commandTimeoutJob?.cancel()
            pendingCommand.value = null
            showCommandOutcome(CommandState.OfflineBlocked)
            return
        }

        commandTimeoutJob?.cancel()
        pendingCommand.value = command
        commandOutcome.value = null

        repository.updateDesiredState(desired)

        commandTimeoutJob = viewModelScope.launch {
            delay(COMMAND_CONFIRMATION_TIMEOUT_MS)
            if (pendingCommand.value == command) {
                pendingCommand.value = null
                showCommandOutcome(CommandState.TimedOut)
            }
        }
    }

    private fun finishCommand(outcome: CommandState) {
        if (pendingCommand.value == null) return
        commandTimeoutJob?.cancel()
        pendingCommand.value = null
        showCommandOutcome(outcome)
    }

    private fun showCommandOutcome(outcome: CommandState) {
        commandOutcome.value = outcome
        viewModelScope.launch {
            delay(COMMAND_OUTCOME_DISPLAY_MS)
            if (commandOutcome.value == outcome) {
                commandOutcome.value = null
            }
        }
    }

    override fun onCleared() {
        commandTimeoutJob?.cancel()
        super.onCleared()
    }

    fun setPumpPower(on: Boolean) {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = uiState.value.operatingMode.name,
            manualDesired = on,
            clearError = false,
            resetStop = false
        )
        submitCommand(DashboardCommand.ManualPower(on), desired)
    }

    fun setControlMode(mode: OperatingMode) {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = mode.name,
            manualDesired = uiState.value.pumpState is com.smartflow.domain.PumpState.Running,
            countdownStart = false,
            clearError = false,
            resetStop = false
        )
        submitCommand(DashboardCommand.ModeChange(mode), desired)
    }

    fun triggerEmergencyStop() {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = OperatingMode.MANUAL.name,
            emergencyStop = true
        )
        submitCommand(DashboardCommand.EmergencyStop, desired)
    }

    fun startCountdown(durationMin: Int) {
        val validationError = DeviceConfigValidator.validateCountdownDuration(durationMin)
        if (validationError != null) {
            commandTimeoutJob?.cancel()
            pendingCommand.value = null
            showCommandOutcome(CommandState.Rejected(validationError))
            return
        }

        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = OperatingMode.COUNTDOWN.name,
            countdownStart = true,
            countdownDurationMin = durationMin,
            clearError = false,
            resetStop = false
        )
        submitCommand(DashboardCommand.CountdownStart, desired)
    }

    fun stopCountdown() {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = OperatingMode.COUNTDOWN.name,
            countdownStart = false,
            clearError = false,
            resetStop = false
        )
        submitCommand(DashboardCommand.CountdownStop, desired)
    }

    fun clearErrors() {
        val currentDesired = repository.shadowFlow.value.desired
        val desired = currentDesired.copy(
            mode = uiState.value.operatingMode.name,
            clearError = true,
            resetStop = true,
            emergencyStop = false
        )
        submitCommand(DashboardCommand.ClearErrors, desired)
    }

    fun updateConfig(config: DeviceConfig) {
        val validation = DeviceConfigValidator.validate(config)
        if (!validation.isValid) {
            val reason = validation.pumpStartLevelError
                ?: validation.pumpStopLevelError
                ?: validation.dryRunThresholdError
                ?: validation.maxPumpRuntimeError
                ?: "Invalid device configuration."
            commandTimeoutJob?.cancel()
            pendingCommand.value = null
            showCommandOutcome(CommandState.Rejected(reason))
            return
        }

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

    companion object {
        /**
         * App-side confirmation timeout only. It is not a pump safety timeout;
         * firmware remains responsible for physical safety and shutdown.
         */
        private const val COMMAND_CONFIRMATION_TIMEOUT_MS = 15_000L
        private const val COMMAND_OUTCOME_DISPLAY_MS = 1_500L
    }
}
