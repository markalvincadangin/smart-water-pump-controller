package com.smartflow.presentation.components.domain

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.rounded.Info
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.domain.DeviceEvent
import com.smartflow.presentation.components.core.EmptyState
import com.smartflow.ui.theme.LocalSpacing
import java.time.Instant
import java.time.ZoneId
import java.time.format.DateTimeFormatter

@Composable
fun ActivityTimeline(
    events: List<DeviceEvent>,
    modifier: Modifier = Modifier
) {
    val spacing = LocalSpacing.current

    if (events.isEmpty()) {
        EmptyState(
            icon = Icons.Rounded.Info,
            title = "No recent activity",
            description = "Events will appear here when the pump operates or faults occur.",
            modifier = modifier.padding(spacing.large)
        )
    } else {
        LazyColumn(
            modifier = modifier,
            contentPadding = PaddingValues(vertical = spacing.small),
            verticalArrangement = Arrangement.spacedBy(spacing.medium)
        ) {
            items(events) { event ->
                ActivityItem(event = event)
            }
        }
    }
}

@Composable
fun ActivityItem(
    event: DeviceEvent,
    modifier: Modifier = Modifier
) {
    val spacing = LocalSpacing.current

    val color = when (event.severity) {
        "CRITICAL" -> MaterialTheme.colorScheme.error
        "WARNING" -> MaterialTheme.colorScheme.tertiary
        else -> MaterialTheme.colorScheme.primary
    }

    val formatter = DateTimeFormatter.ofPattern("h:mm a").withZone(ZoneId.systemDefault())
    val timeString = formatter.format(Instant.ofEpochMilli(event.timestamp))

    Row(
        modifier = modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(spacing.medium),
        verticalAlignment = Alignment.Top
    ) {
        // Dot indicator
        Box(
            modifier = Modifier
                .padding(top = 6.dp)
                .size(8.dp)
                .clip(CircleShape)
                .background(color)
        )

        Column(
            modifier = Modifier.weight(1f),
            verticalArrangement = Arrangement.spacedBy(spacing.extraSmall)
        ) {
            Text(
                text = event.title,
                style = MaterialTheme.typography.bodyLarge,
                color = MaterialTheme.colorScheme.onSurface,
                fontWeight = FontWeight.Bold
            )
            Text(
                text = timeString,
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.7f)
            )
            if (event.severity == "CRITICAL") {
                Spacer(modifier = Modifier.height(spacing.extraSmall))
            }
            Text(
                text = event.logMessage,
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
    }
}
