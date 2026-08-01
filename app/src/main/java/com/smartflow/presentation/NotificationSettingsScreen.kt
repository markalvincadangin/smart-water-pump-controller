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
import com.smartflow.viewmodel.NotificationSettingsViewModel
import com.smartflow.presentation.components.SmartFlowTopAppBar

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun NotificationSettingsScreen(
    viewModel: NotificationSettingsViewModel,
    onBack: () -> Unit
) {
    val prefs by viewModel.prefs.collectAsState()

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
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(16.dp),
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
            
            Text("Informational Alerts", style = MaterialTheme.typography.labelMedium, modifier = Modifier.padding(start = 8.dp, top = 8.dp))

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Column {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(16.dp),
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
                            .padding(16.dp),
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

            Text("Critical Safety Alerts", style = MaterialTheme.typography.labelMedium, modifier = Modifier.padding(start = 8.dp, top = 8.dp))

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Column {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(16.dp),
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
                            .padding(16.dp),
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
