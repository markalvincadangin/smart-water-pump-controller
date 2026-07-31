package com.smartflow.presentation

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Notifications
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.smartflow.presentation.components.SmartFlowTopAppBar

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DeviceListScreen(
    devices: List<String>,
    hasUnreadNotifications: Boolean,
    onDeviceSelected: (String) -> Unit,
    onAddNewDevice: () -> Unit,
    onManageOwnership: (String) -> Unit
) {
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
                .padding(16.dp)
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
                    verticalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    items(devices) { deviceId ->
                        Card(
                            modifier = Modifier
                                .fillMaxWidth()
                                .clickable { onDeviceSelected(deviceId) },
                            colors = CardDefaults.cardColors(
                                containerColor = MaterialTheme.colorScheme.surfaceVariant
                            ),
                            elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
                        ) {
                            Column(modifier = Modifier.padding(16.dp)) {
                                Text(
                                    text = deviceId,
                                    style = MaterialTheme.typography.titleMedium,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant
                                )
                                Spacer(modifier = Modifier.height(12.dp))
                                Row(
                                    modifier = Modifier.fillMaxWidth(),
                                    horizontalArrangement = Arrangement.End
                                ) {
                                    OutlinedButton(onClick = { onManageOwnership(deviceId) }) {
                                        Text("Manage Ownership")
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
