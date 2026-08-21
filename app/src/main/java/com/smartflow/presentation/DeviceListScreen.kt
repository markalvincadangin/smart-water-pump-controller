package com.smartflow.presentation

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.data.repository.FirebaseDeviceRepository
import com.smartflow.domain.ConnectionState
import com.smartflow.domain.DeviceShadow
import com.smartflow.domain.OperatingMode
import com.smartflow.domain.PumpState
import com.smartflow.domain.Telemetry
import com.smartflow.domain.TelemetryValue
import com.smartflow.presentation.components.SmartFlowTopAppBar
import com.smartflow.ui.theme.LocalSpacing
import java.time.Duration
import java.time.Instant

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DeviceListScreen(
    devices: List<String>,
    hasUnreadNotifications: Boolean,
    onDeviceSelected: (String) -> Unit,
    onAddNewDevice: () -> Unit,
    onManageOwnership: (String) -> Unit
) {
    val spacing = LocalSpacing.current

    Scaffold(
        topBar = {
            SmartFlowTopAppBar()
        },
        floatingActionButton = {
            FloatingActionButton(onClick = onAddNewDevice, containerColor = MaterialTheme.colorScheme.primary) {
                Icon(Icons.Default.Add, contentDescription = "Add New Device", tint = MaterialTheme.colorScheme.onPrimary)
            }
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .padding(spacing.medium)
        ) {
            if (devices.isEmpty()) {
                Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Text(
                        "No devices claimed yet.\nTap + to add one.",
                        style = MaterialTheme.typography.bodyLarge,
                        color = MaterialTheme.colorScheme.onBackground,
                        textAlign = androidx.compose.ui.text.style.TextAlign.Center
                    )
                }
            } else {
                LazyColumn(
                    modifier = Modifier.fillMaxSize(),
                    verticalArrangement = Arrangement.spacedBy(spacing.medium)
                ) {
                    items(devices) { deviceId ->
                        DeviceCard(
                            deviceId = deviceId,
                            onClick = { onDeviceSelected(deviceId) },
                            onManageOwnership = { onManageOwnership(deviceId) }
                        )
                    }
                }
            }
        }
    }
}

@Composable
fun DeviceCard(
    deviceId: String,
    onClick: () -> Unit,
    onManageOwnership: () -> Unit
) {
    val repository = remember(deviceId) { FirebaseDeviceRepository(deviceId) }
    
    val connection by repository.connectionFlow.collectAsState(initial = ConnectionState.DISCONNECTED)
    val shadow by repository.shadowFlow.collectAsState(initial = DeviceShadow())
    val telemetry by repository.telemetryFlow.collectAsState(initial = Telemetry())

    val desiredMode = when (shadow.desired.mode) {
        "MANUAL" -> OperatingMode.MANUAL
        "COUNTDOWN" -> OperatingMode.COUNTDOWN
        else -> OperatingMode.AUTO
    }
    
    val currentMode = when (shadow.reported.runMode) {
        "MANUAL", "MANUAL_ON", "MANUAL_OFF", "MANUAL_COOLDOWN" -> OperatingMode.MANUAL
        "COUNTDOWN" -> OperatingMode.COUNTDOWN
        "AUTO", "SMART", "ECO" -> OperatingMode.AUTO
        "IDLE", "ERROR" -> desiredMode
        else -> desiredMode
    }
    
    val pumpState = when {
        connection == ConnectionState.DISCONNECTED -> PumpState.Offline
        shadow.reported.isError || shadow.reported.isOverflowError -> PumpState.Error
        shadow.reported.emergencyStopLatched -> PumpState.Interlocked
        shadow.reported.isRunning -> PumpState.Running
        else -> PumpState.Idle
    }

    val spacing = LocalSpacing.current

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clickable { onClick() },
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        ),
        elevation = CardDefaults.cardElevation(defaultElevation = 0.dp),
        shape = MaterialTheme.shapes.large
    ) {
        Column(
            modifier = Modifier.padding(spacing.medium),
            verticalArrangement = Arrangement.spacedBy(spacing.medium)
        ) {
            // Header: Device ID & Online Status
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = deviceId,
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(4.dp)
                ) {
                    val statusColor = if (connection == ConnectionState.CONNECTED) {
                        MaterialTheme.colorScheme.primary
                    } else {
                        MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f)
                    }
                    val statusText = if (connection == ConnectionState.CONNECTED) "Online" else "Offline"
                    
                    Box(
                        modifier = Modifier
                            .size(8.dp)
                            .background(statusColor, shape = androidx.compose.foundation.shape.CircleShape)
                    )
                    Text(
                        text = statusText,
                        style = MaterialTheme.typography.labelMedium,
                        color = statusColor
                    )
                }
            }

            HorizontalDivider(color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.1f))

            // Operational Summary
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                // Pump Status
                Column {
                    Text(
                        text = "Pump",
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.7f)
                    )
                    Spacer(modifier = Modifier.height(2.dp))
                    Text(
                        text = when (pumpState) {
                            is PumpState.Running -> "Running"
                            is PumpState.Idle -> "Stopped"
                            is PumpState.Error -> "Error"
                            is PumpState.Interlocked -> "E-Stop"
                            is PumpState.Offline -> "Offline"
                            else -> "Unknown"
                        },
                        style = MaterialTheme.typography.bodyLarge,
                        fontWeight = FontWeight.Medium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }

                // Mode
                Column {
                    Text(
                        text = "Mode",
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.7f)
                    )
                    Spacer(modifier = Modifier.height(2.dp))
                    Text(
                        text = when (currentMode) {
                            OperatingMode.AUTO -> "Auto"
                            OperatingMode.MANUAL -> "Manual"
                            OperatingMode.COUNTDOWN -> "Timer"
                        },
                        style = MaterialTheme.typography.bodyLarge,
                        fontWeight = FontWeight.Medium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }

                // Tank
                Column(horizontalAlignment = Alignment.End) {
                    Text(
                        text = "Tank",
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.7f)
                    )
                    Spacer(modifier = Modifier.height(2.dp))
                    val tankLevelText = when (val wl = telemetry.waterLevel) {
                        is TelemetryValue.Available -> "${wl.value}%"
                        is TelemetryValue.Stale -> "${wl.lastKnownValue}%"
                        is TelemetryValue.Unavailable -> "--"
                    }
                    Text(
                        text = tankLevelText,
                        style = MaterialTheme.typography.bodyLarge,
                        fontWeight = FontWeight.Medium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }

            // Timestamp
            val timestamp = when (val wl = telemetry.waterLevel) {
                is TelemetryValue.Available -> wl.timestamp
                is TelemetryValue.Stale -> wl.timestamp
                else -> null
            }
            
            var timeAgoText by remember { mutableStateOf("Updated just now") }
            
            LaunchedEffect(timestamp) {
                if (timestamp != null) {
                    while (true) {
                        val duration = Duration.between(timestamp, Instant.now())
                        timeAgoText = when {
                            duration.seconds < 60 -> "Updated just now"
                            duration.toMinutes() < 60 -> "Updated ${duration.toMinutes()} min ago"
                            duration.toHours() < 24 -> "Updated ${duration.toHours()} hours ago"
                            else -> "Updated ${duration.toDays()} days ago"
                        }
                        kotlinx.coroutines.delay(10000)
                    }
                } else {
                    timeAgoText = "Never updated"
                }
            }

            Text(
                text = timeAgoText,
                style = MaterialTheme.typography.labelSmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.5f)
            )

            // Actions
            OutlinedButton(
                onClick = { onManageOwnership() },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("Manage device")
            }
        }
    }
}
