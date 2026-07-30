package com.smartflow.presentation.components

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.domain.DeviceEvent
import com.smartflow.presentation.theme.AmberWarning
import com.smartflow.presentation.theme.GrayText
import com.smartflow.presentation.theme.RedError
import java.text.SimpleDateFormat
import java.util.*

@Composable
fun ActivityPanel(
    events: List<DeviceEvent> = emptyList(),
    modifier: Modifier = Modifier
) {
    Card(
        modifier = modifier
            .fillMaxWidth()
            .height(200.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        )
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(16.dp)
        ) {
            Text(
                text = "Activity Log",
                style = MaterialTheme.typography.bodyMedium,
                color = GrayText
            )
            Spacer(modifier = Modifier.height(8.dp))

            if (events.isEmpty()) {
                Text(
                    text = "No recent activity.",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurface
                )
            } else {
                val dateFormat = SimpleDateFormat("HH:mm:ss", Locale.getDefault())
                LazyColumn(modifier = Modifier.fillMaxSize()) {
                    items(events) { event ->
                        val timeString = if (event.timestamp > 0L) {
                            if (event.timestamp > 1000000000000L) {
                                dateFormat.format(Date(event.timestamp))
                            } else {
                                val seconds = event.timestamp / 1000
                                val mins = seconds / 60
                                val remainingSecs = seconds % 60
                                String.format(Locale.US, "Up %02d:%02d", mins, remainingSecs)
                            }
                        } else ""
                        
                        val color = when (event.severity) {
                            "ERROR" -> RedError
                            "WARN" -> AmberWarning
                            else -> MaterialTheme.colorScheme.onSurface
                        }
                        
                        Text(
                            text = "[$timeString] [${event.category}] ${event.message}",
                            style = MaterialTheme.typography.bodyMedium,
                            fontWeight = if (event.severity == "ERROR") FontWeight.Bold else FontWeight.Normal,
                            color = color,
                            modifier = Modifier.padding(vertical = 4.dp)
                        )
                    }
                }
            }
        }
    }
}
