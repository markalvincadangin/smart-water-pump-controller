package com.smartflow.presentation

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import android.Manifest
import android.os.Build
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import android.content.Intent
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import androidx.compose.ui.platform.LocalContext
import com.smartflow.viewmodel.ProvisioningState
import com.smartflow.viewmodel.ProvisioningViewModel
import com.smartflow.presentation.components.SmartFlowTopAppBar

@Composable
fun ProvisioningScreen(viewModel: ProvisioningViewModel, onProvisioningSuccess: () -> Unit, onBack: () -> Unit) {
    val state by viewModel.provisioningState.collectAsState()

    var ssid by remember { mutableStateOf("") }
    var password by remember { mutableStateOf("") }

    val permissions = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT, Manifest.permission.ACCESS_FINE_LOCATION)
    } else {
        arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
    }

    val context = LocalContext.current
    val bluetoothManager = context.getSystemService(android.content.Context.BLUETOOTH_SERVICE) as? BluetoothManager
    val bluetoothAdapter = bluetoothManager?.adapter

    val enableBluetoothLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == android.app.Activity.RESULT_OK) {
            viewModel.startScanning()
        }
    }

    val permissionLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.RequestMultiplePermissions()
    ) { result ->
        if (result.values.all { it }) {
            if (bluetoothAdapter?.isEnabled == true) {
                viewModel.startScanning()
            } else {
                enableBluetoothLauncher.launch(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
            }
        }
    }

    Scaffold(
        topBar = {
            SmartFlowTopAppBar(
                title = "Add Device",
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
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            when (val s = state) {
            is ProvisioningState.Idle -> {
                Button(onClick = { permissionLauncher.launch(permissions) }) {
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
                Button(onClick = { viewModel.scanWifiNetworks(s.macAddress) }) {
                    Text("Scan Wi-Fi Networks")
                }
                Spacer(modifier = Modifier.height(8.dp))
                OutlinedButton(onClick = { viewModel.claimOwnershipPairing(s.macAddress) }) {
                    Text("Claim nearby transfer or release")
                }
            }
            is ProvisioningState.ScanningWifi -> {
                CircularProgressIndicator()
                Spacer(modifier = Modifier.height(16.dp))
                Text("Scanning Wi-Fi Networks...")
            }
            is ProvisioningState.WifiListReceived -> {
                Text("Select your Wi-Fi Network", style = MaterialTheme.typography.titleMedium)
                Spacer(modifier = Modifier.height(8.dp))

                if (s.isScanning) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        CircularProgressIndicator(modifier = Modifier.size(16.dp), strokeWidth = 2.dp)
                        Spacer(modifier = Modifier.width(8.dp))
                        Text("Finding networks...", style = MaterialTheme.typography.bodySmall)
                    }
                    Spacer(modifier = Modifier.height(8.dp))
                }

                var expanded by remember { mutableStateOf(false) }
                Box {
                    Button(onClick = { expanded = true }) {
                        Text(if (ssid.isEmpty()) "Select Network" else ssid)
                    }
                    DropdownMenu(
                        expanded = expanded,
                        onDismissRequest = { expanded = false }
                    ) {
                        s.networks.forEach { network ->
                            DropdownMenuItem(
                                text = {
                                    val lock = if (network.auth != "OPEN") "🔒 " else ""
                                    Text("$lock${network.ssid} (${network.rssi} dBm)")
                                },
                                onClick = {
                                    ssid = network.ssid
                                    expanded = false
                                }
                            )
                        }
                        DropdownMenuItem(
                            text = { Text("Other (Manual Entry)") },
                            onClick = {
                                ssid = ""
                                expanded = false
                            }
                        )
                    }
                }
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
                Spacer(modifier = Modifier.height(16.dp))
                Button(
                    onClick = { viewModel.provisionDevice(s.macAddress, ssid, password) },
                    enabled = ssid.isNotEmpty() && (!s.isScanning || s.networks.isNotEmpty())
                ) {
                    Text("Provision Device")
                }
            }
            is ProvisioningState.Provisioning -> {
                CircularProgressIndicator()
                Spacer(modifier = Modifier.height(16.dp))
                Text("Sending credentials to device...")
            }
            is ProvisioningState.OwnershipPairing -> {
                CircularProgressIndicator()
                Spacer(modifier = Modifier.height(16.dp))
                Text("Retrieving the secure nearby ownership proof...")
            }
            is ProvisioningState.ProvisioningUpdate -> {
                CircularProgressIndicator()
                Spacer(modifier = Modifier.height(16.dp))
                Text("Device status: ${s.message}")
            }
            is ProvisioningState.WaitingForCloud -> {
                CircularProgressIndicator()
                Spacer(modifier = Modifier.height(16.dp))
                Text("Connecting device to SmartFlow Cloud…")
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    "Checking secure registration (${s.attempt}/${s.maxAttempts})",
                    style = MaterialTheme.typography.bodySmall
                )
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    "The device has left Bluetooth to join Wi-Fi. Keep this screen open.",
                    style = MaterialTheme.typography.bodySmall
                )
            }
            is ProvisioningState.Success -> {
                Text("Provisioning Successful!")
                Spacer(modifier = Modifier.height(8.dp))
                Text("Device ${s.claimToken} is now registered to your account.", style = MaterialTheme.typography.titleMedium)
                Spacer(modifier = Modifier.height(16.dp))
                Button(onClick = { onProvisioningSuccess() }) {
                    Text("Continue to Dashboard")
                }
            }
            is ProvisioningState.Error -> {
                Text("Error: ${s.message}", color = MaterialTheme.colorScheme.error)
                Spacer(modifier = Modifier.height(16.dp))
                if (s.canRetryCloudClaim) {
                    Button(onClick = { viewModel.retryPendingCloudClaim() }) {
                        Text("Retry cloud registration")
                    }
                    Spacer(modifier = Modifier.height(8.dp))
                    OutlinedButton(onClick = { viewModel.startScanning() }) {
                        Text("Start provisioning again")
                    }
                } else {
                    Button(onClick = { viewModel.startScanning() }) {
                        Text("Retry")
                    }
                }
            }
        }
    }
}

}
