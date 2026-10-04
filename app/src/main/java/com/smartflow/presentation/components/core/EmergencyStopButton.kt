package com.smartflow.presentation.components.core

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.size
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import com.smartflow.ui.theme.LocalSpacing

@Composable
fun EmergencyStopButton(
    text: String,
    isConnected: Boolean,
    isPending: Boolean,
    isLatched: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val spacing = LocalSpacing.current
    val isEnabled = isConnected && !isPending && !isLatched

    val accessibilityDescription = when {
        isLatched -> "Emergency stop is active. Pump is interlocked."
        isPending -> "Emergency stop command is being sent."
        !isConnected -> "Emergency stop is unavailable because the device is offline."
        else -> "Emergency stop. Stops the pump and activates the safety interlock."
    }

    val accessibilityState = when {
        isLatched -> "Active"
        isPending -> "Stopping"
        !isConnected -> "Unavailable"
        else -> "Ready"
    }

    Button(
        onClick = onClick,
        enabled = isEnabled,
        colors = ButtonDefaults.buttonColors(
            containerColor = MaterialTheme.colorScheme.error,
            contentColor = MaterialTheme.colorScheme.onError
        ),
        modifier = modifier.semantics {
            role = Role.Button
            contentDescription = accessibilityDescription
            stateDescription = accessibilityState
        },
        shape = MaterialTheme.shapes.medium
    ) {
        Row(
            horizontalArrangement = Arrangement.spacedBy(spacing.small),
            verticalAlignment = Alignment.CenterVertically
        ) {
            if (isPending) {
                CircularProgressIndicator(
                    modifier = Modifier.size(16.dp),
                    strokeWidth = 2.dp,
                    color = MaterialTheme.colorScheme.onError
                )
            }
            Text(
                text = when {
                    isPending -> "Stopping..."
                    isLatched -> "E-STOP ACTIVE"
                    else -> text
                },
                style = MaterialTheme.typography.labelLarge
            )
        }
    }
}
