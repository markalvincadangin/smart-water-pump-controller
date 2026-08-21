package com.smartflow.presentation.components.core

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.size
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.smartflow.domain.CommandState
import com.smartflow.ui.theme.LocalSpacing

@Composable
fun EmergencyStopButton(
    text: String,
    commandState: CommandState,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val spacing = LocalSpacing.current

    val isPending = commandState is CommandState.Pending || commandState is CommandState.Accepted
    val isEnabled = commandState !is CommandState.Pending && 
                    commandState !is CommandState.Accepted && 
                    commandState !is CommandState.OfflineBlocked

    Button(
        onClick = onClick,
        enabled = isEnabled,
        colors = ButtonDefaults.buttonColors(
            containerColor = MaterialTheme.colorScheme.error,
            contentColor = MaterialTheme.colorScheme.onError
        ),
        modifier = modifier,
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
                text = if (isPending) "Stopping..." else text,
                style = MaterialTheme.typography.labelLarge
            )
        }
    }
}
