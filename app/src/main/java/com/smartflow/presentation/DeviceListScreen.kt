package com.smartflow.presentation

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

@Composable
fun DeviceListScreen(
    devices: List<String>,
    onDeviceSelected: (String) -> Unit,
    onAddNewDevice: () -> Unit
) {
    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp)
    ) {
        Text("My Devices", style = MaterialTheme.typography.headlineMedium)
        Spacer(modifier = Modifier.height(16.dp))

        if (devices.isEmpty()) {
            Text("No devices claimed yet.")
        } else {
            LazyColumn(modifier = Modifier.weight(1f)) {
                items(devices) { deviceId ->
                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(vertical = 8.dp)
                            .clickable { onDeviceSelected(deviceId) }
                    ) {
                        Text(
                            text = deviceId,
                            modifier = Modifier.padding(16.dp),
                            style = MaterialTheme.typography.titleMedium
                        )
                    }
                }
            }
        }

        Spacer(modifier = Modifier.height(16.dp))
        Button(
            onClick = { onAddNewDevice() },
            modifier = Modifier.align(Alignment.CenterHorizontally)
        ) {
            Text("Add New Device")
        }
    }
}
