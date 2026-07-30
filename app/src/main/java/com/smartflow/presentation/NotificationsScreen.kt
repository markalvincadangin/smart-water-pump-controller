package com.smartflow.presentation

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.viewmodel.NotificationsViewModel
import java.text.SimpleDateFormat
import java.util.*

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun NotificationsScreen(
    viewModel: NotificationsViewModel,
    onNavigateSettings: () -> Unit,
    onBack: () -> Unit
) {
    val events by viewModel.events.collectAsState()
    val prefs by viewModel.prefs.collectAsState()
    
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Notifications") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
                actions = {
                    IconButton(onClick = { viewModel.markAllAsRead() }) {
                        Icon(Icons.Default.Delete, contentDescription = "Mark all as read")
                    }
                    IconButton(onClick = onNavigateSettings) {
                        Icon(Icons.Default.Settings, contentDescription = "Settings")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.onPrimaryContainer,
                    navigationIconContentColor = MaterialTheme.colorScheme.onPrimaryContainer,
                    actionIconContentColor = MaterialTheme.colorScheme.onPrimaryContainer
                )
            )
        }
    ) { paddingValues ->
        if (events.isEmpty()) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(paddingValues),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = "No notifications",
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        } else {
            val dateFormat = SimpleDateFormat("MMM dd, HH:mm", Locale.getDefault())
            
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(paddingValues),
                contentPadding = PaddingValues(16.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                items(events) { (deviceId, event) ->
                    val isUnread = event.timestamp > prefs.lastReadTimestamp
                    val timeString = if (event.timestamp > 1000000000000L) {
                        dateFormat.format(Date(event.timestamp))
                    } else {
                        "Unknown time"
                    }
                    
                    val color = when (event.severity) {
                        "ERROR" -> MaterialTheme.colorScheme.error
                        "WARN" -> MaterialTheme.colorScheme.tertiary
                        else -> MaterialTheme.colorScheme.onSurfaceVariant
                    }
                    
                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        colors = CardDefaults.cardColors(
                            containerColor = if (isUnread) MaterialTheme.colorScheme.surfaceVariant else MaterialTheme.colorScheme.surface
                        ),
                        elevation = CardDefaults.cardElevation(defaultElevation = if (isUnread) 2.dp else 0.dp)
                    ) {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(16.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            if (isUnread) {
                                Box(
                                    modifier = Modifier
                                        .size(8.dp)
                                        .background(MaterialTheme.colorScheme.primary, shape = androidx.compose.foundation.shape.CircleShape)
                                )
                                Spacer(modifier = Modifier.width(12.dp))
                            } else {
                                Spacer(modifier = Modifier.width(20.dp))
                            }
                            
                            Column(modifier = Modifier.weight(1f)) {
                                Text(
                                    text = deviceId,
                                    style = MaterialTheme.typography.labelMedium,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.7f)
                                )
                                Spacer(modifier = Modifier.height(4.dp))
                                Text(
                                    text = "[${event.category}] ${event.message}",
                                    style = MaterialTheme.typography.bodyMedium,
                                    fontWeight = if (event.severity == "ERROR") FontWeight.Bold else FontWeight.Normal,
                                    color = color
                                )
                                Spacer(modifier = Modifier.height(4.dp))
                                Text(
                                    text = timeString,
                                    style = MaterialTheme.typography.labelSmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.5f)
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}
