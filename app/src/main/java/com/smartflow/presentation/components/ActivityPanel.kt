package com.smartflow.presentation.components

import androidx.compose.foundation.layout.*
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.smartflow.domain.DeviceEvent
import com.smartflow.presentation.components.domain.ActivityTimeline
import com.smartflow.ui.theme.LocalSpacing

@Composable
fun ActivityPanel(
    events: List<DeviceEvent> = emptyList(),
    modifier: Modifier = Modifier
) {
    val spacing = LocalSpacing.current

    Card(
        modifier = modifier
            .fillMaxWidth()
            .height(280.dp), // Increased height for better UX
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        )
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(spacing.medium)
        ) {
            Text(
                text = "Recent Activity",
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            Spacer(modifier = Modifier.height(spacing.mediumSmall))

            ActivityTimeline(
                events = events,
                modifier = Modifier.fillMaxSize()
            )
        }
    }
}
