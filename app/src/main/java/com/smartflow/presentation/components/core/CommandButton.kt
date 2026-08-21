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
fun CommandButton(
    text: String,
    commandState: CommandState,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    isPrimary: Boolean = true,
    pendingText: String? = null
) {
    val spacing = LocalSpacing.current

    val isPending = commandState is CommandState.Pending || commandState is CommandState.Accepted
    val isEnabled = commandState !is CommandState.Pending && 
                    commandState !is CommandState.Accepted && 
                    commandState !is CommandState.OfflineBlocked && 
                    commandState !is CommandState.InterlockBlocked

    val colors = if (isPrimary) {
        ButtonDefaults.buttonColors(
            containerColor = MaterialTheme.colorScheme.primary,
            contentColor = MaterialTheme.colorScheme.onPrimary
        )
    } else {
        ButtonDefaults.outlinedButtonColors()
    }

    if (isPrimary) {
        Button(
            onClick = onClick,
            enabled = isEnabled,
            colors = colors,
            modifier = modifier,
            shape = MaterialTheme.shapes.medium
        ) {
            ButtonContent(text, isPending, pendingText ?: "Pending...", spacing)
        }
    } else {
        OutlinedButton(
            onClick = onClick,
            enabled = isEnabled,
            colors = colors,
            modifier = modifier,
            shape = MaterialTheme.shapes.medium
        ) {
            ButtonContent(text, isPending, pendingText ?: "Pending...", spacing)
        }
    }
}

@Composable
private fun ButtonContent(text: String, isPending: Boolean, pendingText: String, spacing: com.smartflow.ui.theme.Spacing) {
    Row(
        horizontalArrangement = Arrangement.spacedBy(spacing.small),
        verticalAlignment = Alignment.CenterVertically
    ) {
        if (isPending) {
            CircularProgressIndicator(
                modifier = Modifier.size(16.dp),
                strokeWidth = 2.dp,
                color = MaterialTheme.colorScheme.onPrimary
            )
        }
        Text(
            text = if (isPending) pendingText else text,
            style = MaterialTheme.typography.labelLarge
        )
    }
}
