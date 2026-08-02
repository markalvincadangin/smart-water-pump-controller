package com.smartflow.presentation.components

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.domain.ConnectionState
import com.smartflow.domain.ControlMode


import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.runtime.*
import androidx.compose.ui.hapticfeedback.HapticFeedbackType
import androidx.compose.ui.platform.LocalHapticFeedback

@Composable
fun ControlPanel(
    mode: ControlMode,
    desiredMode: ControlMode,
    isPumpRunning: Boolean,
    lockoutActive: Boolean,
    connectionState: ConnectionState,
    isManualDesired: Boolean,
    isCountdownStartDesired: Boolean,
    lastFaultMessage: String,
    onModeChanged: (ControlMode) -> Unit,
    onEmergencyStop: () -> Unit,
    onPowerToggle: (Boolean) -> Unit,
    onCountdownStart: (Int) -> Unit,
    onClearError: () -> Unit,
    maxRuntimeLimitMins: Int = 120,
    modifier: Modifier = Modifier
) {
    val isConnected = connectionState == ConnectionState.CONNECTED
    var countdownDuration by remember { mutableFloatStateOf(30f) }
    val haptic = LocalHapticFeedback.current

    Column(
        modifier = modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        // Mode Selector (AUTO / MANUAL)
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(24.dp))
                .background(MaterialTheme.colorScheme.surfaceVariant),
            horizontalArrangement = Arrangement.SpaceEvenly
        ) {
            ModeButton(
                text = "AUTO",
                isSelected = desiredMode == ControlMode.AUTO,
                isPending = desiredMode == ControlMode.AUTO && mode != ControlMode.AUTO,
                isEnabled = isConnected,
                onClick = { onModeChanged(ControlMode.AUTO) },
                modifier = Modifier.weight(1f)
            )
            ModeButton(
                text = "MANUAL",
                isSelected = desiredMode == ControlMode.MANUAL,
                isPending = desiredMode == ControlMode.MANUAL && mode != ControlMode.MANUAL,
                isEnabled = isConnected,
                onClick = { onModeChanged(ControlMode.MANUAL) },
                modifier = Modifier.weight(1f)
            )
            ModeButton(
                text = "TIMER",
                isSelected = desiredMode == ControlMode.COUNTDOWN,
                isPending = desiredMode == ControlMode.COUNTDOWN && mode != ControlMode.COUNTDOWN,
                isEnabled = isConnected,
                onClick = { onModeChanged(ControlMode.COUNTDOWN) },
                modifier = Modifier.weight(1f)
            )
        }

        val isCountdownRunning = mode == ControlMode.COUNTDOWN && isPumpRunning

        if (mode == ControlMode.COUNTDOWN) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(MaterialTheme.colorScheme.surfaceVariant, RoundedCornerShape(16.dp))
                    .padding(16.dp)
            ) {
                if (isCountdownRunning) {
                    // Timer is active — show only a STOP TIMER button.
                    Text(
                        "Timer running",
                        style = MaterialTheme.typography.bodyMedium,
                        modifier = Modifier.padding(bottom = 12.dp)
                    )
                    Button(
                        onClick = {
                            haptic.performHapticFeedback(HapticFeedbackType.LongPress)
                            onEmergencyStop()
                        },
                        enabled = isConnected && !lockoutActive,
                        shape = RoundedCornerShape(24.dp),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = MaterialTheme.colorScheme.error,
                            contentColor = MaterialTheme.colorScheme.onError
                        ),
                        modifier = Modifier.fillMaxWidth().height(56.dp)
                    ) {
                        Text("STOP TIMER", fontWeight = FontWeight.Bold)
                    }
                } else {
                    // Timer is in standby — show duration picker and START TIMER.
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text("Duration", style = MaterialTheme.typography.bodyMedium)
                        Text("${countdownDuration.toInt()} mins", style = MaterialTheme.typography.labelLarge)
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
                    Button(
                        onClick = {
                            haptic.performHapticFeedback(HapticFeedbackType.LongPress)
                            onCountdownStart(countdownDuration.toInt())
                        },
                        enabled = isConnected && !lockoutActive && !(isCountdownStartDesired && !isPumpRunning),
                        shape = RoundedCornerShape(24.dp),
                        colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.primary),
                        modifier = Modifier.fillMaxWidth().height(56.dp)
                    ) {
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.Center
                        ) {
                            if (isCountdownStartDesired && !isPumpRunning) {
                                androidx.compose.material3.CircularProgressIndicator(
                                    modifier = Modifier.size(16.dp),
                                    color = MaterialTheme.colorScheme.onPrimary,
                                    strokeWidth = 2.dp
                                )
                                Spacer(modifier = Modifier.width(8.dp))
                            }
                            Text("START TIMER", fontWeight = FontWeight.Bold)
                        }
                    }
                }
            }
        }

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            if (lockoutActive) {
                // Clear Error Button
                Button(
                    onClick = onClearError,
                    enabled = isConnected,
                    shape = RoundedCornerShape(24.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = MaterialTheme.colorScheme.tertiary,
                        contentColor = MaterialTheme.colorScheme.onSurface
                    ),
                    modifier = Modifier
                        .weight(1f)
                        .height(56.dp)
                ) {
                    Text("CLEAR FAULT", fontWeight = FontWeight.Bold)
                }
            } else {
                // Power Toggle (Only enabled in MANUAL mode)
                val isPendingManual = mode == ControlMode.MANUAL && (isPumpRunning != isManualDesired)
                Button(
                    onClick = { 
                        haptic.performHapticFeedback(HapticFeedbackType.LongPress)
                        onPowerToggle(!isPumpRunning) 
                    },
                    enabled = isConnected && mode == ControlMode.MANUAL && !isPendingManual,
                    shape = RoundedCornerShape(24.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = if (isPumpRunning) MaterialTheme.colorScheme.surfaceVariant else MaterialTheme.colorScheme.primary,
                        contentColor = if (isPumpRunning) MaterialTheme.colorScheme.onSurface else MaterialTheme.colorScheme.onPrimary
                    ),
                    modifier = Modifier
                        .weight(1f)
                        .height(56.dp) // Ensures > 48dp minimum touch target
                ) {
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.Center
                    ) {
                        if (isPendingManual) {
                            androidx.compose.material3.CircularProgressIndicator(
                                modifier = Modifier.size(16.dp),
                                color = if (isPumpRunning) MaterialTheme.colorScheme.onSurface else MaterialTheme.colorScheme.onPrimary,
                                strokeWidth = 2.dp
                            )
                            Spacer(modifier = Modifier.width(8.dp))
                        }
                        Text(
                            text = if (isPumpRunning) "STOP PUMP" else "START PUMP",
                            fontWeight = FontWeight.Bold
                        )
                    }
                }
            }

            // E-STOP (High visibility Red pill button)
            Button(
                onClick = { 
                    haptic.performHapticFeedback(HapticFeedbackType.LongPress)
                    onEmergencyStop() 
                },
                enabled = isConnected,
                shape = RoundedCornerShape(24.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = MaterialTheme.colorScheme.error,
                    contentColor = MaterialTheme.colorScheme.onError
                ),
                modifier = Modifier
                    .weight(1f)
                    .height(56.dp) // Ensures > 48dp minimum touch target
            ) {
                Text(text = "E-STOP", fontWeight = FontWeight.Bold)
            }
        }
    }
}

@Composable
private fun ModeButton(
    text: String,
    isSelected: Boolean,
    isPending: Boolean = false,
    isEnabled: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val backgroundColor = if (isSelected) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.surfaceVariant
    val contentColor = if (isSelected) MaterialTheme.colorScheme.onPrimary else MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)
    
    Box(
        modifier = modifier
            .height(56.dp) // Ensures > 48dp minimum touch target
            .clip(RoundedCornerShape(24.dp))
            .background(backgroundColor)
            .clickable(enabled = isEnabled && !isPending, onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.Center
        ) {
            if (isPending) {
                androidx.compose.material3.CircularProgressIndicator(
                    modifier = Modifier.size(16.dp),
                    color = contentColor,
                    strokeWidth = 2.dp
                )
                Spacer(modifier = Modifier.width(8.dp))
            }
            Text(
                text = text,
                color = contentColor,
                fontWeight = FontWeight.Bold
            )
        }
    }
}
