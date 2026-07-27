package com.smartflow.presentation

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.smartflow.viewmodel.DashboardViewModel

@Composable
fun DashboardScreen(viewModel: DashboardViewModel) {
    val device by viewModel.deviceState.collectAsState()

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.Center,
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
        }
    }
}
