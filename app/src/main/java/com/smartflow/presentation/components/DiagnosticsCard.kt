package com.smartflow.presentation.components

import androidx.compose.foundation.layout.*
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.smartflow.domain.ConnectionState


@Composable
fun DiagnosticsCard(
    connectionState: ConnectionState,
    modifier: Modifier = Modifier
) {
    Card(
        modifier = modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(
                text = "System Diagnostics",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            
            val (statusText, color) = when (connectionState) {
                ConnectionState.CONNECTED -> "ONLINE" to MaterialTheme.colorScheme.secondary
                ConnectionState.CONNECTING -> "CONNECTING..." to MaterialTheme.colorScheme.onSurfaceVariant
                ConnectionState.DISCONNECTED -> "OFFLINE" to MaterialTheme.colorScheme.error
            }

            Text(
                text = statusText,
                style = MaterialTheme.typography.labelLarge,
                color = color
            )
        }
    }
}
