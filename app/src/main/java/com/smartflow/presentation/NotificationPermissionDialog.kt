package com.smartflow.presentation

import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import com.smartflow.service.NotificationPermissionPrompt

@Composable
fun NotificationPermissionDialog(
    state: NotificationPermissionPrompt,
    onRequestPermission: () -> Unit,
    onOpenSettings: () -> Unit,
    onDismiss: () -> Unit
) {
    val firstRequest = state == NotificationPermissionPrompt.EXPLAIN

    AlertDialog(
        onDismissRequest = onDismiss,
        title = {
            Text(
                if (firstRequest) "Allow SmartFlow notifications?"
                else "Notifications are turned off"
            )
        },
        text = {
            Text(
                if (firstRequest) {
                    "SmartFlow can notify you about pump safety interventions and important pump activity. Critical safety notifications are useful when you are not watching the app."
                } else {
                    "Android notifications are currently blocked for SmartFlow. Open system notification settings to enable them."
                }
            )
        },
        confirmButton = {
            TextButton(
                onClick = if (firstRequest) onRequestPermission else onOpenSettings
            ) {
                Text(if (firstRequest) "Continue" else "Open Settings")
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text(if (firstRequest) "Not now" else "Later")
            }
        }
    )
}
