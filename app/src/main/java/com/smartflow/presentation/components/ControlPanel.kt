package com.smartflow.presentation.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.LiveRegionMode
import androidx.compose.ui.semantics.liveRegion
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.hapticfeedback.HapticFeedbackType
import androidx.compose.ui.platform.LocalHapticFeedback
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import java.time.LocalTime
import java.time.format.DateTimeFormatter
import com.smartflow.domain.CommandState
import com.smartflow.domain.ConnectionState
import com.smartflow.domain.OperatingMode
import com.smartflow.domain.PumpState
import com.smartflow.presentation.components.core.CommandButton
import com.smartflow.presentation.components.core.EmergencyStopButton
import com.smartflow.presentation.components.settings.ModeSelector
import com.smartflow.ui.theme.LocalSpacing

@Composable
fun ControlPanel(
    operatingMode: OperatingMode,
    desiredMode: OperatingMode,
    pumpState: PumpState,
    connectionState: ConnectionState,
    commandState: CommandState,
    lastFaultMessage: String,
    onModeChanged: (OperatingMode) -> Unit,
    onEmergencyStop: () -> Unit,
    onPowerToggle: (Boolean) -> Unit,
    onCountdownStart: (Int) -> Unit,
    onCountdownStop: () -> Unit,
    onClearError: () -> Unit,
    maxRuntimeLimitMins: Int = 120,
    countdownRemainingSec: Int = 0,
    countdownDurationMin: Int = 0,
    modifier: Modifier = Modifier
) {
    val isConnected = connectionState == ConnectionState.CONNECTED
    var countdownDuration by remember { mutableFloatStateOf(30f) }
    val haptic = LocalHapticFeedback.current
    val spacing = LocalSpacing.current

    val isPumpRunning = pumpState is PumpState.Running || pumpState is PumpState.Starting
    val lockoutActive = pumpState is PumpState.Interlocked || pumpState is PumpState.Error
    val isPendingManual = operatingMode == OperatingMode.MANUAL && (commandState is CommandState.Pending || commandState is CommandState.Accepted)
    val isCountdownStartDesired = desiredMode == OperatingMode.COUNTDOWN && (commandState is CommandState.Pending || commandState is CommandState.Accepted)

    Column(
        modifier = modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(spacing.medium)
    ) {
        // Mode Selector (AUTO / MANUAL / COUNTDOWN)
        ModeSelector(
            operatingMode = operatingMode,
            desiredMode = desiredMode,
            commandState = commandState,
            onModeSelected = { mode ->
                if (isConnected) onModeChanged(mode)
            }
        )

        val isTimerActive = operatingMode == OperatingMode.COUNTDOWN && countdownRemainingSec > 0

        if (operatingMode == OperatingMode.COUNTDOWN) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(MaterialTheme.colorScheme.surfaceVariant, RoundedCornerShape(spacing.medium))
                    .padding(spacing.medium)
            ) {
                if (isTimerActive) {
                    val minutesRemaining = countdownRemainingSec / 60
                    val secondsRemaining = countdownRemainingSec % 60
                    
                    val now = LocalTime.now()
                    
                    var startTime by remember { mutableStateOf<LocalTime?>(null) }
                    var endTime by remember { mutableStateOf<LocalTime?>(null) }
                    
                    LaunchedEffect(isTimerActive, countdownDurationMin) {
                        if (isTimerActive && startTime == null) {
                            val end = now.plusSeconds(countdownRemainingSec.toLong())
                            endTime = end
                            startTime = end.minusMinutes(countdownDurationMin.toLong())
                        } else if (!isTimerActive) {
                            startTime = null
                            endTime = null
                        }
                    }

                    Text(
                        "Timer active",
                        style = MaterialTheme.typography.titleMedium,
                        color = MaterialTheme.colorScheme.primary,
                        modifier = Modifier.padding(bottom = spacing.small)
                    )
                    
                    Text(
                        String.format("%02d:%02d remaining", minutesRemaining, secondsRemaining),
                        style = MaterialTheme.typography.headlineMedium,
                        fontWeight = FontWeight.Bold,
                        modifier = Modifier.padding(bottom = spacing.small)
                    )
                    
                    val formatter = DateTimeFormatter.ofPattern("h:mm a")
                    if (startTime != null && endTime != null) {
                        Text(
                            "Started ${startTime!!.format(formatter)}",
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                        Text(
                            "Ends ${endTime!!.format(formatter)}",
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                            modifier = Modifier.padding(bottom = spacing.medium)
                        )
                    } else {
                        Spacer(modifier = Modifier.height(spacing.medium))
                    }
                    
                    CommandButton(
                        text = "STOP TIMER",
                        commandState = commandState,
                        onClick = {
                            haptic.performHapticFeedback(HapticFeedbackType.LongPress)
                            onCountdownStop()
                        },
                        isPrimary = false,
                        modifier = Modifier.fillMaxWidth().height(56.dp)
                    )
                } else {
                    // Timer is in standby — show duration picker and START TIMER.
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text("Duration", style = MaterialTheme.typography.bodyMedium)
                        Text("${countdownDuration.toInt()} min", style = MaterialTheme.typography.labelLarge)
                    }
                    Slider(
                        value = countdownDuration.coerceAtMost(maxRuntimeLimitMins.toFloat()),
                        onValueChange = { countdownDuration = it },
                        valueRange = 1f..maxRuntimeLimitMins.toFloat(),
                        steps = (maxRuntimeLimitMins - 1).coerceAtLeast(0),
                        colors = SliderDefaults.colors(
                            thumbColor = MaterialTheme.colorScheme.primary,
                            activeTrackColor = MaterialTheme.colorScheme.primary
                        )
                    )
                    Row(
                        modifier = Modifier.fillMaxWidth().padding(bottom = spacing.medium),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text("Minimum 1 min", style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
                        Text("Maximum $maxRuntimeLimitMins min", style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
                    }
                    CommandButton(
                        text = "START TIMER",
                        commandState = if (isCountdownStartDesired && !isPumpRunning) CommandState.Pending else commandState,
                        onClick = {
                            haptic.performHapticFeedback(HapticFeedbackType.LongPress)
                            onCountdownStart(countdownDuration.toInt())
                        },
                        modifier = Modifier.fillMaxWidth().height(56.dp)
                    )
                }
            }
        }

        if (lockoutActive && lastFaultMessage.isNotEmpty()) {
            Text(
                text = "System Fault: $lastFaultMessage",
                color = MaterialTheme.colorScheme.error,
                style = MaterialTheme.typography.bodyLarge,
                fontWeight = FontWeight.Bold,
                modifier = Modifier
                    .fillMaxWidth()
                    .semantics { liveRegion = LiveRegionMode.Assertive }
                    .padding(vertical = spacing.small)
            )
        }

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(spacing.medium),
            verticalAlignment = Alignment.Top
        ) {
            if (lockoutActive) {
                // Clear Error Button
                CommandButton(
                    text = "CLEAR FAULT",
                    commandState = commandState,
                    onClick = onClearError,
                    isPrimary = false,
                    modifier = Modifier.weight(1f).height(56.dp)
                )
            } else {
                // Power Toggle (Only enabled in MANUAL mode)
                val manualCommandState = if (!isConnected || operatingMode != OperatingMode.MANUAL) CommandState.OfflineBlocked else commandState
                
                PrimaryCommandArea(
                    isPumpRunning = isPumpRunning,
                    commandState = manualCommandState,
                    onClick = {
                        haptic.performHapticFeedback(HapticFeedbackType.LongPress)
                        onPowerToggle(!isPumpRunning)
                    },
                    modifier = Modifier.weight(1f)
                )
            }

            // E-STOP (High visibility Red pill button)
            EmergencyStopButton(
                text = "E-STOP",
                commandState = commandState,
                onClick = {
                    haptic.performHapticFeedback(HapticFeedbackType.LongPress)
                    onEmergencyStop()
                },
                modifier = Modifier.weight(1f).height(56.dp)
            )
        }
    }
}

@Composable
private fun PrimaryCommandArea(
    isPumpRunning: Boolean,
    commandState: CommandState,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val spacing = LocalSpacing.current

    val buttonText = when (commandState) {
        is CommandState.Pending, is CommandState.Accepted -> if (isPumpRunning) "STOPPING PUMP..." else "STARTING PUMP..."
        else -> if (isPumpRunning) "STOP PUMP" else "START PUMP"
    }

    val subtitleText = when (commandState) {
        is CommandState.Ready -> if (isPumpRunning) "Pump running" else ""
        is CommandState.Pending -> "Awaiting device confirmation"
        is CommandState.Accepted -> "Command accepted\nWaiting for pump feedback..."
        is CommandState.Rejected -> "Start rejected\n${commandState.reason}"
        is CommandState.TimedOut -> "Command timed out\nPump state could not be confirmed"
        is CommandState.OfflineBlocked -> "Manual control disabled"
        is CommandState.InterlockBlocked -> "Blocked by interlock"
        is CommandState.Completed -> if (isPumpRunning) "Pump running" else ""
    }

    val subtitleColor = when (commandState) {
        is CommandState.Rejected, is CommandState.TimedOut -> MaterialTheme.colorScheme.error
        is CommandState.Pending, is CommandState.Accepted -> MaterialTheme.colorScheme.primary
        else -> MaterialTheme.colorScheme.onSurfaceVariant
    }

    Column(modifier = modifier) {
        CommandButton(
            text = if (isPumpRunning) "STOP PUMP" else "START PUMP",
            pendingText = buttonText,
            commandState = commandState,
            onClick = onClick,
            isPrimary = !isPumpRunning,
            modifier = Modifier
                .fillMaxWidth()
                .height(56.dp)
        )
        if (subtitleText.isNotEmpty()) {
            Text(
                text = subtitleText,
                style = MaterialTheme.typography.bodySmall,
                color = subtitleColor,
                modifier = Modifier.padding(top = spacing.small)
            )
        }
    }
}
