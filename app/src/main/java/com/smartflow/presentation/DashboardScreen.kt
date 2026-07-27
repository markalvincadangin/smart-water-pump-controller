package com.smartflow.presentation

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowUp
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import com.smartflow.viewmodel.DashboardViewModel

@Composable
fun DashboardScreen(viewModel: DashboardViewModel) {
    val device by viewModel.deviceState.collectAsState()
    val events by viewModel.eventsState.collectAsState()

    var showDiagnostics by remember { mutableStateOf(false) }
    var showEvents by remember { mutableStateOf(false) }
    var showResetDialog by remember { mutableStateOf(false) }
    var resetError by remember { mutableStateOf<String?>(null) }

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp).verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.Top,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        if (device == null) {
            CircularProgressIndicator()
            Spacer(modifier = Modifier.height(16.dp))
            Text("Loading Device State...")
        } else {
            val telemetry = device?.telemetry
            val reportedPumpState = device?.shadow?.reported?.pumpState ?: false
            
            Text("Device: ${device?.id}", style = MaterialTheme.typography.headlineMedium)
            Spacer(modifier = Modifier.height(16.dp))
            
            Card(modifier = Modifier.fillMaxWidth().padding(8.dp)) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Text("Telemetry", style = MaterialTheme.typography.titleLarge)
                    Spacer(modifier = Modifier.height(8.dp))
                    Text("Water Level: ${telemetry?.waterLevel ?: 0.0}%")
                    Text("Flow Rate: ${telemetry?.flowRate ?: 0.0} L/min")
                }
            }
            
            Spacer(modifier = Modifier.height(16.dp))
            
            Card(modifier = Modifier.fillMaxWidth().padding(8.dp)) {
                Row(
                    modifier = Modifier.padding(16.dp).fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text("Pump State: ${if (reportedPumpState) "ON" else "OFF"}", style = MaterialTheme.typography.titleMedium)
                    Button(onClick = { viewModel.togglePump(reportedPumpState) }) {
                        Text(if (reportedPumpState) "TURN OFF" else "TURN ON")
                    }
                }
            }
            
            Spacer(modifier = Modifier.height(16.dp))
            
            // Diagnostics Section
            val diagnostics = device?.diagnostics
            Card(modifier = Modifier.fillMaxWidth().padding(8.dp)) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text("Diagnostics", style = MaterialTheme.typography.titleLarge)
                        IconButton(onClick = { showDiagnostics = !showDiagnostics }) {
                            Icon(if (showDiagnostics) Icons.Default.KeyboardArrowUp else Icons.Default.KeyboardArrowDown, contentDescription = null)
                        }
                    }
                    if (showDiagnostics) {
                        Spacer(modifier = Modifier.height(8.dp))
                        Text("Free Heap: ${diagnostics?.freeHeap ?: 0} bytes")
                        Text("Wi-Fi RSSI: ${diagnostics?.wifiRSSI ?: 0} dBm")
                        Text("Restart Reason: ${diagnostics?.restartReason ?: "Unknown"}")
                    }
                }
            }
            
            Spacer(modifier = Modifier.height(16.dp))
            
            // Events Section
            Card(modifier = Modifier.fillMaxWidth().padding(8.dp)) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text("Event Log", style = MaterialTheme.typography.titleLarge)
                        IconButton(onClick = { showEvents = !showEvents }) {
                            Icon(if (showEvents) Icons.Default.KeyboardArrowUp else Icons.Default.KeyboardArrowDown, contentDescription = null)
                        }
                    }
                    if (showEvents) {
                        Spacer(modifier = Modifier.height(8.dp))
                        if (events.isEmpty()) {
                            Text("No events found.")
                        } else {
                            val sdf = SimpleDateFormat("MMM dd, HH:mm:ss", Locale.getDefault())
                            events.forEach { event ->
                                val timeStr = sdf.format(Date(event.timestamp))
                                val severityColor = when (event.severity.uppercase()) {
                                    "ERROR"  -> MaterialTheme.colorScheme.error
                                    "WARN"   -> MaterialTheme.colorScheme.tertiary
                                    else     -> MaterialTheme.colorScheme.onSurfaceVariant
                                }
                                Column(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                                    Row(
                                        modifier = Modifier.fillMaxWidth(),
                                        horizontalArrangement = Arrangement.SpaceBetween
                                    ) {
                                        Text(
                                            "[${event.severity}]",
                                            style = MaterialTheme.typography.labelSmall,
                                            color = severityColor
                                        )
                                        Text(
                                            timeStr,
                                            style = MaterialTheme.typography.labelSmall,
                                            color = MaterialTheme.colorScheme.onSurfaceVariant
                                        )
                                    }
                                    if (event.category.isNotEmpty() || event.code.isNotEmpty()) {
                                        Text(
                                            "${event.category} · ${event.code}",
                                            style = MaterialTheme.typography.labelSmall,
                                            color = MaterialTheme.colorScheme.onSurfaceVariant
                                        )
                                    }
                                    Text(event.message, style = MaterialTheme.typography.bodySmall)
                                }
                                HorizontalDivider(modifier = Modifier.padding(vertical = 2.dp))
                            }
                        }
                    }
                }
            }

            Spacer(modifier = Modifier.height(16.dp))

            // Factory Reset Section
            resetError?.let { err ->
                Text(err, color = MaterialTheme.colorScheme.error, style = MaterialTheme.typography.bodySmall)
                Spacer(modifier = Modifier.height(8.dp))
            }
            Button(
                onClick = { showResetDialog = true },
                colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.error)
            ) {
                Text("FACTORY RESET DEVICE")
            }

            // Confirmation dialog
            if (showResetDialog) {
                AlertDialog(
                    onDismissRequest = { showResetDialog = false },
                    title = { Text("Factory Reset") },
                    text = { Text("Are you sure? This will erase all device data and remove it from your account. This action cannot be undone.") },
                    confirmButton = {
                        TextButton(
                            onClick = {
                                showResetDialog = false
                                viewModel.factoryReset(
                                    onSuccess = { /* navigate back */ },
                                    onError = { msg -> resetError = msg }
                                )
                            }
                        ) {
                            Text("RESET", color = MaterialTheme.colorScheme.error)
                        }
                    },
                    dismissButton = {
                        TextButton(onClick = { showResetDialog = false }) {
                            Text("Cancel")
                        }
                    }
                )
            }
        }
    }
}
