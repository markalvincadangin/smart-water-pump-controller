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
import com.smartflow.presentation.theme.CyanPrimary
import com.smartflow.presentation.theme.RedError

import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.runtime.*

@Composable
fun ControlPanel(
    mode: ControlMode,
    isPumpRunning: Boolean,
    lockoutActive: Boolean,
    connectionState: ConnectionState,
    onModeChanged: (ControlMode) -> Unit,
    onEmergencyStop: () -> Unit,
    onPowerToggle: (Boolean) -> Unit,
    onCountdownStart: (Int) -> Unit,
    onClearError: () -> Unit,
    modifier: Modifier = Modifier
) {
    val isConnected = connectionState == ConnectionState.CONNECTED
    var countdownDuration by remember { mutableFloatStateOf(30f) }

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
                isSelected = mode == ControlMode.AUTO,
                isEnabled = isConnected,
                onClick = { onModeChanged(ControlMode.AUTO) },
                modifier = Modifier.weight(1f)
            )
            ModeButton(
                text = "MANUAL",
                isSelected = mode == ControlMode.MANUAL,
                isEnabled = isConnected,
                onClick = { onModeChanged(ControlMode.MANUAL) },
                modifier = Modifier.weight(1f)
            )
            ModeButton(
                text = "TIMER",
                isSelected = mode == ControlMode.COUNTDOWN,
                isEnabled = isConnected,
                onClick = { onModeChanged(ControlMode.COUNTDOWN) },
                modifier = Modifier.weight(1f)
            )
        }

        if (mode == ControlMode.COUNTDOWN) {
            Column(
                modifier = Modifier.fillMaxWidth().background(MaterialTheme.colorScheme.surfaceVariant, RoundedCornerShape(16.dp)).padding(16.dp)
            ) {
                Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                    Text("Duration", style = MaterialTheme.typography.bodyMedium)
                    Text("${countdownDuration.toInt()} mins", style = MaterialTheme.typography.labelLarge)
                }
                Slider(
                    value = countdownDuration,
                    onValueChange = { countdownDuration = it },
                    valueRange = 1f..120f,
                    steps = 119,
                    colors = SliderDefaults.colors(thumbColor = CyanPrimary, activeTrackColor = CyanPrimary)
                )
                Button(
                    onClick = { onCountdownStart(countdownDuration.toInt()) },
                    enabled = isConnected && !lockoutActive,
                    shape = RoundedCornerShape(24.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = CyanPrimary),
                    modifier = Modifier.fillMaxWidth().height(56.dp)
                ) {
                    Text("START TIMER", fontWeight = FontWeight.Bold)
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
                        containerColor = com.smartflow.presentation.theme.AmberWarning,
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
                Button(
                    onClick = { onPowerToggle(!isPumpRunning) },
                    enabled = isConnected && mode == ControlMode.MANUAL,
                    shape = RoundedCornerShape(24.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = if (isPumpRunning) MaterialTheme.colorScheme.surfaceVariant else CyanPrimary,
                        contentColor = if (isPumpRunning) MaterialTheme.colorScheme.onSurface else MaterialTheme.colorScheme.onPrimary
                    ),
                    modifier = Modifier
                        .weight(1f)
                        .height(56.dp) // Ensures > 48dp minimum touch target
                ) {
                    Text(
                        text = if (isPumpRunning) "STOP PUMP" else "START PUMP",
                        fontWeight = FontWeight.Bold
                    )
                }
            }

            // E-STOP (High visibility Red pill button)
            Button(
                onClick = onEmergencyStop,
                enabled = isConnected,
                shape = RoundedCornerShape(24.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = RedError,
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
    isEnabled: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val backgroundColor = if (isSelected) CyanPrimary else MaterialTheme.colorScheme.surfaceVariant
    val contentColor = if (isSelected) MaterialTheme.colorScheme.onPrimary else MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)
    
    Box(
        modifier = modifier
            .height(56.dp) // Ensures > 48dp minimum touch target
            .clip(RoundedCornerShape(24.dp))
            .background(backgroundColor)
            .clickable(enabled = isEnabled, onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = text,
            color = contentColor,
            fontWeight = FontWeight.Bold
        )
    }
}
