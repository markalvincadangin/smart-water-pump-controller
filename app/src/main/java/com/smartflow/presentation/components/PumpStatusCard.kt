package com.smartflow.presentation.components

import androidx.compose.foundation.layout.*
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import com.smartflow.domain.ConnectionState
import com.smartflow.domain.OperatingMode
import com.smartflow.domain.PumpState
import com.smartflow.domain.TelemetryValue
import com.smartflow.presentation.components.core.StatusChip
import com.smartflow.presentation.components.core.StatusChipState
import com.smartflow.ui.theme.LocalSpacing

@Composable
fun PumpStatusCard(
    pumpState: PumpState,
    operatingMode: OperatingMode,
    flowRate: TelemetryValue<Float>,
    connectionState: ConnectionState,
    modifier: Modifier = Modifier
) {
    val isStale = connectionState != ConnectionState.CONNECTED
    val spacing = LocalSpacing.current

    Card(
        modifier = modifier
            .fillMaxWidth()
            .clearAndSetSemantics {
                contentDescription = if (isStale) {
                    "Pump status unknown due to stale data"
                } else {
                    val status = pumpState.javaClass.simpleName
                    val flowText = when (flowRate) {
                        is TelemetryValue.Stale -> "Flow rate ${String.format("%.1f", flowRate.lastKnownValue)} liters per minute, but data is stale"
                        is TelemetryValue.Unavailable -> "Flow rate error"
                        is TelemetryValue.Available -> "Flow rate ${String.format("%.1f", flowRate.value)} liters per minute"
                    }
                    "Pump is $status in $operatingMode mode. $flowText."
                }
            },
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        )
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(spacing.large)
        ) {
            // Row 1: "PUMP STATUS" and StatusChip
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = "PUMP STATUS",
                    style = MaterialTheme.typography.labelMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                
                val chipState = if (isStale || pumpState is PumpState.Offline) {
                    StatusChipState.Offline
                } else if (pumpState is PumpState.Error || pumpState is PumpState.Interlocked) {
                    StatusChipState.Critical
                } else if (pumpState is PumpState.Running || pumpState is PumpState.Starting) {
                    StatusChipState.Healthy
                } else {
                    StatusChipState.Healthy // Idle
                }

                StatusChip(
                    state = chipState,
                    label = if (isStale) "UNKNOWN" else pumpState.javaClass.simpleName.uppercase()
                )
            }
            
            Spacer(modifier = Modifier.height(spacing.medium))

            // Row 2: Main Pump State text (e.g. "Running")
            Text(
                text = if (isStale) "Unknown" else pumpState.javaClass.simpleName,
                style = MaterialTheme.typography.headlineMedium,
                color = MaterialTheme.colorScheme.onSurface
            )

            Spacer(modifier = Modifier.height(spacing.small))

            // Row 3: Operating mode
            val modeText = when (operatingMode) {
                OperatingMode.AUTO -> "Auto mode"
                OperatingMode.MANUAL -> "Manual mode"
                OperatingMode.COUNTDOWN -> "Timer mode"
            }
            Text(
                text = modeText,
                style = MaterialTheme.typography.bodyLarge,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )

            Spacer(modifier = Modifier.height(spacing.extraSmall))

            // Row 4: Flow Rate inline
            val flowRateText = when (flowRate) {
                is TelemetryValue.Unavailable -> "Flow -- L/min"
                is TelemetryValue.Available -> String.format("Flow %.1f L/min", flowRate.value)
                is TelemetryValue.Stale -> String.format("Flow %.1f L/min", flowRate.lastKnownValue)
            }
            Text(
                text = flowRateText,
                style = MaterialTheme.typography.bodyLarge,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )

            // Row 5: Freshness Indicator
            if (isStale) {
                Spacer(modifier = Modifier.height(spacing.small))
                val minutesAgo = if (flowRate is TelemetryValue.Stale) {
                    java.time.Duration.between(flowRate.timestamp, java.time.Instant.now()).toMinutes()
                } else {
                    0L
                }
                Text(
                    text = if (minutesAgo > 0) "Last updated $minutesAgo min ago" else "Last updated just now",
                    style = MaterialTheme.typography.labelMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            } else if (flowRate is TelemetryValue.Available) {
                Spacer(modifier = Modifier.height(spacing.small))
                val secondsAgo = java.time.Duration.between(flowRate.timestamp, java.time.Instant.now()).seconds
                val updateText = if (secondsAgo < 60) "Last updated just now" else "Last updated ${secondsAgo / 60} min ago"
                Text(
                    text = updateText,
                    style = MaterialTheme.typography.labelMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}
