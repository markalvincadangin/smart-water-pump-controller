package com.smartflow.presentation

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import com.smartflow.viewmodel.ProvisioningState
import com.smartflow.viewmodel.ProvisioningViewModel

@Composable
fun ProvisioningScreen(viewModel: ProvisioningViewModel, onProvisioningSuccess: () -> Unit) {
    val state by viewModel.provisioningState.collectAsState()

    var ssid by remember { mutableStateOf("") }
    var password by remember { mutableStateOf("") }

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.Center,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        when (val s = state) {
            is ProvisioningState.Idle -> {
                Button(onClick = { viewModel.startScanning() }) {
                    Text("Scan for SmartFlow Device")
                }
            }
            is ProvisioningState.Scanning -> {
                CircularProgressIndicator()
                Spacer(modifier = Modifier.height(16.dp))
                Text("Scanning for devices...")
            }
            is ProvisioningState.DeviceFound -> {
                Text("Device Found: ${s.macAddress}")
                Spacer(modifier = Modifier.height(16.dp))
                OutlinedTextField(
                    value = ssid,
                    onValueChange = { ssid = it },
                    label = { Text("Wi-Fi SSID") }
                )
                Spacer(modifier = Modifier.height(8.dp))
                OutlinedTextField(
                    value = password,
                    onValueChange = { password = it },
                    label = { Text("Wi-Fi Password") },
                    visualTransformation = PasswordVisualTransformation()
                )
                Spacer(modifier = Modifier.height(8.dp))
                Spacer(modifier = Modifier.height(16.dp))
                Button(onClick = { 
                    viewModel.provisionDevice(s.macAddress, ssid, password) 
                }) {
                    Text("Provision Device")
                }
            }
            is ProvisioningState.Provisioning -> {
                CircularProgressIndicator()
                Spacer(modifier = Modifier.height(16.dp))
                Text("Sending credentials to device...")
            }
            is ProvisioningState.Success -> {
                Text("Provisioning Successful!")
                Spacer(modifier = Modifier.height(8.dp))
                Text("Your Claim Token: ${s.claimToken}", style = MaterialTheme.typography.titleMedium)
                Spacer(modifier = Modifier.height(16.dp))
                Button(onClick = { onProvisioningSuccess() }) {
                    Text("Continue to Dashboard")
                }
            }
            is ProvisioningState.Error -> {
                Text("Error: ${s.message}", color = MaterialTheme.colorScheme.error)
                Spacer(modifier = Modifier.height(16.dp))
                Button(onClick = { viewModel.startScanning() }) {
                    Text("Retry")
                }
            }
        }
    }
}
