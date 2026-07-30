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

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun NotificationSettingsScreen(
    viewModel: NotificationSettingsViewModel,
    onBack: () -> Unit
) {
    val prefs by viewModel.prefs.collectAsState()

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Notification Settings") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.onPrimaryContainer,
                    navigationIconContentColor = MaterialTheme.colorScheme.onPrimaryContainer
                )
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

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text("Do Not Disturb", style = MaterialTheme.typography.titleMedium)
                            Text("Mute non-critical notifications during these hours.", style = MaterialTheme.typography.bodyMedium)
                        }
                        Switch(
                            checked = prefs.dndEnabled,
                            onCheckedChange = { viewModel.updatePrefs(prefs.copy(dndEnabled = it)) }
                        )
                    }

                    if (prefs.dndEnabled) {
                        Spacer(modifier = Modifier.height(16.dp))
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Text("Start Time: ${String.format("%02d:00", prefs.dndStartHour)}", style = MaterialTheme.typography.bodyMedium)
                            // A real app would have a TimePicker here. 
                            Text("End Time: ${String.format("%02d:00", prefs.dndEndHour)}", style = MaterialTheme.typography.bodyMedium)
                        }
                    }
                }
            }
        }
    }
}
