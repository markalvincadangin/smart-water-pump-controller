package com.smartflow.presentation

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DeviceOwnershipScreen(
    deviceId: String,
    onStartTransfer: suspend (String) -> Unit,
    onRelease: suspend () -> Unit,
    onCancelPairing: suspend () -> Unit,
    onRequestWifiRecovery: suspend () -> Unit,
    onOpenProvisioning: () -> Unit,
    onBack: () -> Unit,
) {
    var recipientUid by remember { mutableStateOf("") }
    var message by remember { mutableStateOf("") }
    var busy by remember { mutableStateOf(false) }
    var confirmWifiRecovery by remember { mutableStateOf(false) }
    val scope = rememberCoroutineScope()
    val scrollState = rememberScrollState()

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Manage Device") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(imageVector = Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.onPrimaryContainer
                )
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .verticalScroll(scrollState)
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Text(
                text = "Device ID: $deviceId",
                style = MaterialTheme.typography.titleLarge,
                color = MaterialTheme.colorScheme.onBackground
            )

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
                    Text(
                        text = "Wi-Fi Configuration",
                        style = MaterialTheme.typography.titleMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    Text(
                        text = "This safely turns the pump OFF and restarts the device into BLE Wi-Fi setup. Ownership and safety latches remain unchanged.",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    OutlinedButton(
                        enabled = !busy,
                        onClick = { confirmWifiRecovery = true },
                        modifier = Modifier.fillMaxWidth(),
                    ) { Text("Reconfigure Wi-Fi") }
                }
            }

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
                    Text(
                        text = "Ownership Transfer",
                        style = MaterialTheme.typography.titleMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    Text(
                        text = "Transfers and release require the nearby, online device. It will turn the pump OFF and advertise BLE for five minutes. Ownership remains unchanged unless the replacement completes local pairing.",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    
                    OutlinedTextField(
                        value = recipientUid,
                        onValueChange = { recipientUid = it },
                        label = { Text("Recipient Firebase UID") },
                        modifier = Modifier.fillMaxWidth(),
                        enabled = !busy,
                        singleLine = true
                    )
                    
                    Button(
                        enabled = recipientUid.isNotBlank() && !busy,
                        onClick = {
                            busy = true
                            scope.launch {
                                try {
                                    onStartTransfer(recipientUid.trim())
                                    message = "Transfer requested. Keep the replacement user nearby and complete BLE pairing within five minutes."
                                } catch (error: Exception) {
                                    message = error.localizedMessage ?: "Unable to start transfer."
                                } finally { busy = false }
                            }
                        },
                        modifier = Modifier.fillMaxWidth(),
                    ) { Text("Start transfer") }
                    
                    OutlinedButton(
                        enabled = !busy,
                        onClick = {
                            busy = true
                            scope.launch {
                                try {
                                    onRelease()
                                    message = "Release requested. A nearby eligible replacement user must complete BLE pairing within five minutes."
                                } catch (error: Exception) {
                                    message = error.localizedMessage ?: "Unable to request release."
                                } finally { busy = false }
                            }
                        },
                        modifier = Modifier.fillMaxWidth(),
                    ) { Text("Release to a nearby replacement") }
                    
                    OutlinedButton(
                        enabled = !busy,
                        onClick = {
                            busy = true
                            scope.launch {
                                try {
                                    onCancelPairing()
                                    message = "Ownership pairing cancelled. The device remains registered to you."
                                } catch (error: Exception) {
                                    message = error.localizedMessage ?: "No active ownership pairing to cancel."
                                } finally { busy = false }
                            }
                        },
                        modifier = Modifier.fillMaxWidth(),
                    ) { Text("Cancel active ownership pairing") }
                }
            }

            if (message.isNotBlank()) {
                Surface(
                    color = MaterialTheme.colorScheme.secondaryContainer,
                    shape = MaterialTheme.shapes.medium,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text(
                        text = message,
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSecondaryContainer,
                        modifier = Modifier.padding(16.dp)
                    )
                }
            }
        }
    }

    if (confirmWifiRecovery) {
        AlertDialog(
            onDismissRequest = { if (!busy) confirmWifiRecovery = false },
            title = { Text("Reconfigure Wi-Fi?") },
            text = { Text("The pump will be stopped before the device clears only its local Wi-Fi settings and restarts into Bluetooth setup. Device ownership and safety protections will remain in place.") },
            confirmButton = {
                Button(
                    enabled = !busy,
                    onClick = {
                        busy = true
                        scope.launch {
                            try {
                                onRequestWifiRecovery()
                                confirmWifiRecovery = false
                                onOpenProvisioning()
                            } catch (error: Exception) {
                                confirmWifiRecovery = false
                                message = error.localizedMessage ?: "Unable to request Wi-Fi recovery."
                            } finally { busy = false }
                        }
                    },
                ) { Text(if (busy) "Requesting…" else "Stop pump and continue") }
            },
            dismissButton = {
                OutlinedButton(enabled = !busy, onClick = { confirmWifiRecovery = false }) { Text("Cancel") }
            },
        )
    }
}
