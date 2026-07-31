package com.smartflow.presentation.components

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.WaterDrop
import androidx.compose.material.icons.filled.Wifi
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.smartflow.domain.DeviceEvent

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
            .height(280.dp), // Increased height for better UX
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
                text = "Recent Activity",
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            Spacer(modifier = Modifier.height(12.dp))

            if (events.isEmpty()) {
                Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Text(
                        text = "No recent activity.",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurface
                    )
                }
            } else {
                val dateFormat = SimpleDateFormat("HH:mm", Locale.getDefault())
                LazyColumn(
                    modifier = Modifier.fillMaxSize(),
                    verticalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    items(events) { event ->
                        val timeString = if (event.timestamp > 1000000000000L) {
                            dateFormat.format(Date(event.timestamp))
                        } else {
                            val seconds = event.timestamp / 1000
                            val mins = seconds / 60
                            val remainingSecs = seconds % 60
                            String.format(Locale.US, "Up %02d:%02d", mins, remainingSecs)
                        }
                        
                        val icon = when (event.category) {
                            "PUMP" -> Icons.Default.WaterDrop
                            "SYSTEM" -> Icons.Default.Memory
                            "NETWORK" -> Icons.Default.Wifi
                            else -> Icons.Default.Info
                        }
                        
                        val iconColor = when (event.severity) {
                            "ERROR" -> MaterialTheme.colorScheme.error
                            "WARN" -> MaterialTheme.colorScheme.tertiary
                            else -> MaterialTheme.colorScheme.primary
                        }

                        Row(
                            verticalAlignment = Alignment.Top, // Better for multi-line text
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Surface(
                                shape = androidx.compose.foundation.shape.CircleShape,
                                color = iconColor.copy(alpha = 0.1f),
                                modifier = Modifier.size(32.dp)
                            ) {
                                Box(contentAlignment = Alignment.Center) {
                                    Icon(
                                        imageVector = icon,
                                        contentDescription = event.category,
                                        tint = iconColor,
                                        modifier = Modifier.size(16.dp)
                                    )
                                }
                            }
                            
                            Spacer(modifier = Modifier.width(12.dp))
                            
                            Column(modifier = Modifier.weight(1f)) {
                                Row(
                                    modifier = Modifier.fillMaxWidth(),
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Text(
                                        text = event.title,
                                        style = MaterialTheme.typography.labelSmall,
                                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                                        fontWeight = androidx.compose.ui.text.font.FontWeight.SemiBold
                                    )
                                    Text(
                                        text = timeString,
                                        style = MaterialTheme.typography.labelSmall,
                                        color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.7f)
                                    )
                                }
                                Spacer(modifier = Modifier.height(2.dp))
                                Text(
                                    text = event.logMessage,
                                    style = MaterialTheme.typography.bodyMedium,
                                    color = MaterialTheme.colorScheme.onSurface,
                                    maxLines = 2,
                                    overflow = TextOverflow.Ellipsis
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}
