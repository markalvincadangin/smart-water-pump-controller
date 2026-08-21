package com.smartflow.presentation

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.smartflow.domain.DeviceEvent
import com.smartflow.viewmodel.NotificationsViewModel
import com.smartflow.presentation.components.SmartFlowTopAppBar
import com.smartflow.ui.theme.LocalSpacing
import java.text.SimpleDateFormat
import java.util.*

@OptIn(ExperimentalMaterial3Api::class, ExperimentalFoundationApi::class)
@Composable
fun NotificationsScreen(
    viewModel: NotificationsViewModel,
    onNavigateSettings: () -> Unit,
    onBack: () -> Unit
) {
    val events by viewModel.events.collectAsState()
    val prefs by viewModel.prefs.collectAsState()
    val selectionMode by viewModel.selectionMode.collectAsState()
    val selectedEventIds by viewModel.selectedEventIds.collectAsState()
    val spacing = LocalSpacing.current
    
    Scaffold(
        topBar = {
            if (selectionMode) {
                TopAppBar(
                    title = { Text("${selectedEventIds.size} Selected") },
                    navigationIcon = {
                        IconButton(onClick = { viewModel.clearSelection() }) {
                            Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Cancel selection")
                        }
                    },
                    actions = {
                        IconButton(onClick = { viewModel.selectAll() }) {
                            Icon(Icons.Default.SelectAll, contentDescription = "Select All")
                        }
                        IconButton(onClick = { viewModel.markSelectedAsRead() }) {
                            Icon(Icons.Default.Drafts, contentDescription = "Mark as read")
                        }
                        IconButton(onClick = { viewModel.deleteSelected() }) {
                            Icon(Icons.Default.Delete, contentDescription = "Delete")
                        }
                    },
                    colors = TopAppBarDefaults.topAppBarColors(
                        containerColor = MaterialTheme.colorScheme.secondaryContainer,
                        titleContentColor = MaterialTheme.colorScheme.onSecondaryContainer,
                        navigationIconContentColor = MaterialTheme.colorScheme.onSecondaryContainer,
                        actionIconContentColor = MaterialTheme.colorScheme.onSecondaryContainer
                    )
                )
            } else {
                SmartFlowTopAppBar(
                    title = "Notifications",
                    showBackButton = true,
                    onBackClick = onBack,
                    actions = {
                        IconButton(onClick = { viewModel.markAllAsRead() }) {
                            Icon(Icons.Default.DoneAll, contentDescription = "Mark all as read")
                        }
                        IconButton(onClick = onNavigateSettings) {
                            Icon(Icons.Default.Settings, contentDescription = "Settings")
                        }
                    }
                )
            }
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
                contentPadding = PaddingValues(spacing.medium),
                verticalArrangement = Arrangement.spacedBy(spacing.small)
            ) {
                items(events, key = { it.second.id }) { (deviceId, event) ->
                    val isRead = prefs.readEventIds[event.id] == true || event.timestamp <= prefs.lastReadTimestamp
                    val isUnread = !isRead
                    val isSelected = selectedEventIds.contains(event.id)
                    val timeString = if (event.timestamp > 1000000000000L) {
                        dateFormat.format(Date(event.timestamp))
                    } else {
                        "Unknown time"
                    }
                    
                    val cardColor = when {
                        isSelected -> MaterialTheme.colorScheme.primaryContainer
                        isUnread -> MaterialTheme.colorScheme.surfaceVariant
                        else -> MaterialTheme.colorScheme.surface
                    }

                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .combinedClickable(
                                onClick = {
                                    if (selectionMode) {
                                        viewModel.toggleSelection(event.id)
                                    } else if (isUnread) {
                                        viewModel.toggleSelection(event.id) // Briefly select it to mark as read
                                        viewModel.markSelectedAsRead()
                                    }
                                },
                                onLongClick = {
                                    viewModel.toggleSelection(event.id)
                                }
                            ),
                        colors = CardDefaults.cardColors(containerColor = cardColor),
                        elevation = CardDefaults.cardElevation(defaultElevation = if (isUnread && !isSelected) 2.dp else 0.dp)
                    ) {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(spacing.medium),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            if (selectionMode) {
                                Checkbox(
                                    checked = isSelected,
                                    onCheckedChange = { viewModel.toggleSelection(event.id) },
                                    modifier = Modifier.padding(end = spacing.small)
                                )
                            } else if (isUnread) {
                                Box(
                                    modifier = Modifier
                                        .size(8.dp)
                                        .background(MaterialTheme.colorScheme.primary, shape = androidx.compose.foundation.shape.CircleShape)
                                )
                                Spacer(modifier = Modifier.width(spacing.mediumSmall))
                            } else {
                                Spacer(modifier = Modifier.width(spacing.medium + spacing.extraSmall))
                            }

                            // Icon based on category
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

                            Surface(
                                shape = androidx.compose.foundation.shape.CircleShape,
                                color = iconColor.copy(alpha = 0.1f),
                                modifier = Modifier.size(40.dp)
                            ) {
                                Box(contentAlignment = Alignment.Center) {
                                    Icon(
                                        imageVector = icon,
                                        contentDescription = event.category,
                                        tint = iconColor,
                                        modifier = Modifier.size(24.dp)
                                    )
                                }
                            }
                            
                            Spacer(modifier = Modifier.width(spacing.medium))
                            
                            Column(modifier = Modifier.weight(1f)) {
                                Row(
                                    verticalAlignment = Alignment.CenterVertically,
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    modifier = Modifier.fillMaxWidth()
                                ) {
                                    Text(
                                        text = deviceId,
                                        style = MaterialTheme.typography.labelSmall,
                                        color = MaterialTheme.colorScheme.onSurfaceVariant
                                    )
                                    Text(
                                        text = timeString,
                                        style = MaterialTheme.typography.labelSmall,
                                        color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.7f)
                                    )
                                }
                                
                                Spacer(modifier = Modifier.height(spacing.extraSmall))
                                
                                Text(
                                    text = event.title,
                                    style = MaterialTheme.typography.titleSmall,
                                    fontWeight = FontWeight.SemiBold,
                                    color = if (event.severity == "ERROR") {
                                        MaterialTheme.colorScheme.error
                                    } else {
                                        MaterialTheme.colorScheme.onSurface
                                    }
                                )
                                Spacer(modifier = Modifier.height(spacing.extraSmall))
                                Text(
                                    text = event.notificationMessage,
                                    style = MaterialTheme.typography.bodyMedium,
                                    fontWeight = if (isUnread) FontWeight.Bold else FontWeight.Normal,
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
