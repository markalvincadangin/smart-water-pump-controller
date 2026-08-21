package com.smartflow.presentation.components.core

import androidx.compose.foundation.layout.*
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp
import com.smartflow.domain.TelemetryValue
import com.smartflow.ui.theme.LocalSpacing
import java.time.Duration
import java.time.Instant

@Composable
fun <T> TelemetryMetric(
    label: String,
    telemetry: TelemetryValue<T>,
    formatValue: (T) -> String,
    modifier: Modifier = Modifier,
    isLarge: Boolean = false
) {
    val spacing = LocalSpacing.current

    Column(
        modifier = modifier,
        horizontalAlignment = Alignment.Start,
        verticalArrangement = Arrangement.spacedBy(spacing.extraSmall)
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )

        when (telemetry) {
            is TelemetryValue.Available -> {
                Text(
                    text = formatValue(telemetry.value),
                    style = if (isLarge) MaterialTheme.typography.displayMedium else MaterialTheme.typography.titleLarge,
                    color = MaterialTheme.colorScheme.onSurface,
                    fontWeight = FontWeight.Bold
                )
            }
            is TelemetryValue.Stale -> {
                Text(
                    text = formatValue(telemetry.lastKnownValue),
                    style = if (isLarge) MaterialTheme.typography.displayMedium else MaterialTheme.typography.titleLarge,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    fontWeight = FontWeight.Bold
                )
                val duration = Duration.between(telemetry.timestamp, Instant.now())
                val minutes = duration.toMinutes()
                Text(
                    text = "Last updated $minutes min ago",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.tertiary
                )
            }
            is TelemetryValue.Unavailable -> {
                Text(
                    text = "--",
                    style = if (isLarge) MaterialTheme.typography.displayMedium else MaterialTheme.typography.titleLarge,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    fontWeight = FontWeight.Bold
                )
                Text(
                    text = "Unavailable",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.error
                )
            }
        }
    }
}
