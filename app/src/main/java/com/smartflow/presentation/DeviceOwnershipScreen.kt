package com.smartflow.presentation

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.launch
import androidx.compose.runtime.rememberCoroutineScope

@androidx.compose.runtime.Composable
fun DeviceOwnershipScreen(
    deviceId: String,
    onStartTransfer: suspend (String) -> Unit,
    onRelease: suspend () -> Unit,
    onCancelPairing: suspend () -> Unit,
    onRequestWifiRecovery: suspend () -> Unit,
    onOpenProvisioning: () -> Unit,
) {
    var recipientUid by remember { mutableStateOf("") }
    var message by remember { mutableStateOf("") }
    var busy by remember { mutableStateOf(false) }
    var confirmWifiRecovery by remember { mutableStateOf(false) }
    val scope = rememberCoroutineScope()

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.Center,
    ) {
        Text("Manage $deviceId", style = MaterialTheme.typography.headlineSmall)
        Spacer(Modifier.height(12.dp))
        Text("Transfers and release require the nearby, online device. It will turn the pump OFF and advertise BLE for five minutes. Ownership remains unchanged unless the replacement completes local pairing.")
        Spacer(Modifier.height(20.dp))
        OutlinedButton(
            enabled = !busy,
            onClick = { confirmWifiRecovery = true },
            modifier = Modifier.fillMaxWidth(),
        ) { Text("Reconfigure Wi-Fi") }
        Text(
            "This safely turns the pump OFF and restarts the device into BLE Wi-Fi setup. Ownership and safety latches remain unchanged.",
            style = MaterialTheme.typography.bodySmall,
        )
        Spacer(Modifier.height(20.dp))
        OutlinedTextField(
            value = recipientUid,
            onValueChange = { recipientUid = it },
            label = { Text("Recipient Firebase UID") },
            modifier = Modifier.fillMaxWidth(),
            enabled = !busy,
        )
        Spacer(Modifier.height(8.dp))
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
        Spacer(Modifier.height(12.dp))
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
        Spacer(Modifier.height(12.dp))
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
        if (message.isNotBlank()) {
            Spacer(Modifier.height(16.dp))
            Text(message, style = MaterialTheme.typography.bodyMedium)
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
