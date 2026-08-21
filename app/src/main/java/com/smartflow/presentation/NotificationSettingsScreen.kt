package com.smartflow.presentation

import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.foundation.clickable
import androidx.compose.material.icons.filled.KeyboardArrowLeft
import androidx.compose.material.icons.filled.KeyboardArrowRight
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import com.smartflow.viewmodel.NotificationSettingsViewModel
import com.smartflow.presentation.components.SmartFlowTopAppBar
import com.smartflow.ui.theme.LocalSpacing

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun NotificationSettingsScreen(
    viewModel: NotificationSettingsViewModel,
    onBack: () -> Unit
) {
    val prefs by viewModel.prefs.collectAsState()
    var showTimeDialog by remember { mutableStateOf(false) }
    val spacing = LocalSpacing.current

    Scaffold(
        topBar = {
            SmartFlowTopAppBar(
                title = "Notification Settings",
                showBackButton = true,
                onBackClick = onBack
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .padding(spacing.medium),
            verticalArrangement = Arrangement.spacedBy(spacing.medium)
        ) {
            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(spacing.medium),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Column(modifier = Modifier.weight(1f)) {
                        Text("Enable Push Notifications", style = MaterialTheme.typography.titleMedium)
                        Text("Receive critical alerts and pump status updates.", style = MaterialTheme.typography.bodyMedium)
                    }
                    Switch(
                        checked = prefs.enabled,
                        onCheckedChange = { viewModel.updatePrefs(prefs.copy(enabled = it)) }
                    )
                }
            }
            
            Text("Do Not Disturb", style = MaterialTheme.typography.labelMedium, modifier = Modifier.padding(start = spacing.small, top = spacing.small))

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Column {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(spacing.medium),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text("Quiet Hours", style = MaterialTheme.typography.titleMedium)
                            Text("Silence all non-critical notifications.", style = MaterialTheme.typography.bodyMedium)
                        }
                        Switch(
                            checked = prefs.dndEnabled && prefs.enabled,
                            enabled = prefs.enabled,
                            onCheckedChange = { viewModel.updatePrefs(prefs.copy(dndEnabled = it)) }
                        )
                    }
                    if (prefs.dndEnabled && prefs.enabled) {
                        HorizontalDivider()
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .clickable { showTimeDialog = true }
                                .padding(spacing.medium),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Text("Schedule (Tap to change)", style = MaterialTheme.typography.bodyLarge)
                            Text(
                                String.format("%02d:00 to %02d:00", prefs.dndStartHour, prefs.dndEndHour),
                                style = MaterialTheme.typography.titleMedium,
                                color = MaterialTheme.colorScheme.primary
                            )
                        }
                    }
                }
            }
            
            if (showTimeDialog) {
                var tempStart by remember { mutableStateOf(prefs.dndStartHour) }
                var tempEnd by remember { mutableStateOf(prefs.dndEndHour) }

                AlertDialog(
                    onDismissRequest = { showTimeDialog = false },
                    title = { Text("Set Quiet Hours") },
                    text = {
                        Column(verticalArrangement = Arrangement.spacedBy(spacing.medium)) {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Text("Start Time", modifier = Modifier.weight(1f))
                                IconButton(onClick = { tempStart = if (tempStart == 0) 23 else tempStart - 1 }) {
                                    Icon(Icons.Default.KeyboardArrowLeft, contentDescription = "Decrease")
                                }
                                Text(String.format("%02d:00", tempStart), style = MaterialTheme.typography.titleMedium)
                                IconButton(onClick = { tempStart = if (tempStart == 23) 0 else tempStart + 1 }) {
                                    Icon(Icons.Default.KeyboardArrowRight, contentDescription = "Increase")
                                }
                            }
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Text("End Time", modifier = Modifier.weight(1f))
                                IconButton(onClick = { tempEnd = if (tempEnd == 0) 23 else tempEnd - 1 }) {
                                    Icon(Icons.Default.KeyboardArrowLeft, contentDescription = "Decrease")
                                }
                                Text(String.format("%02d:00", tempEnd), style = MaterialTheme.typography.titleMedium)
                                IconButton(onClick = { tempEnd = if (tempEnd == 23) 0 else tempEnd + 1 }) {
                                    Icon(Icons.Default.KeyboardArrowRight, contentDescription = "Increase")
                                }
                            }
                        }
                    },
                    confirmButton = {
                        TextButton(
                            onClick = {
                                viewModel.updatePrefs(prefs.copy(dndStartHour = tempStart, dndEndHour = tempEnd))
                                showTimeDialog = false
                            }
                        ) {
                            Text("Save")
                        }
                    },
                    dismissButton = {
                        TextButton(onClick = { showTimeDialog = false }) {
                            Text("Cancel")
                        }
                    }
                )
            }

            Text("Informational Alerts", style = MaterialTheme.typography.labelMedium, modifier = Modifier.padding(start = spacing.small, top = spacing.small))

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Column {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(spacing.medium),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text("Pump Started", style = MaterialTheme.typography.titleMedium)
                            Text("Receive a notification when the pump starts running.", style = MaterialTheme.typography.bodyMedium)
                        }
                        Switch(
                            checked = prefs.pumpStartedAlert && prefs.enabled,
                            enabled = prefs.enabled,
                            onCheckedChange = { viewModel.updatePrefs(prefs.copy(pumpStartedAlert = it)) }
                        )
                    }
                    HorizontalDivider()
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(spacing.medium),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text("Low Tank Level", style = MaterialTheme.typography.titleMedium)
                            Text("Receive a notification when the water level drops below the threshold.", style = MaterialTheme.typography.bodyMedium)
                        }
                        Switch(
                            checked = prefs.lowLevelAlert && prefs.enabled,
                            enabled = prefs.enabled,
                            onCheckedChange = { viewModel.updatePrefs(prefs.copy(lowLevelAlert = it)) }
                        )
                    }
                }
            }

            Text("Critical Safety Alerts", style = MaterialTheme.typography.labelMedium, modifier = Modifier.padding(start = spacing.small, top = spacing.small))

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Column {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(spacing.medium),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text("Dry-Run Protection", style = MaterialTheme.typography.titleMedium)
                            Text("Critical safety alert (Cannot be disabled)", style = MaterialTheme.typography.bodyMedium, color = MaterialTheme.colorScheme.error)
                        }
                        Switch(
                            checked = true,
                            enabled = false,
                            onCheckedChange = { }
                        )
                    }
                    HorizontalDivider()
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(spacing.medium),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text("Overflow Protection", style = MaterialTheme.typography.titleMedium)
                            Text("Critical safety alert (Cannot be disabled)", style = MaterialTheme.typography.bodyMedium, color = MaterialTheme.colorScheme.error)
                        }
                        Switch(
                            checked = true,
                            enabled = false,
                            onCheckedChange = { }
                        )
                    }
                }
            }
        }
    }
}
