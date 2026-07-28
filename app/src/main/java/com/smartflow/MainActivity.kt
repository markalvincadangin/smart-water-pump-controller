package com.smartflow

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import com.google.firebase.auth.FirebaseAuth
import com.polidea.rxandroidble3.RxBleClient
import com.smartflow.data.BleProvisioningClient
import com.smartflow.data.DeviceRepository
import com.smartflow.data.FirebaseCloudStore
import com.smartflow.presentation.DashboardScreen
import com.smartflow.presentation.DeviceListScreen
import com.smartflow.presentation.LoginScreen
import com.smartflow.presentation.ProvisioningScreen
import com.smartflow.viewmodel.DashboardViewModel
import com.smartflow.viewmodel.ProvisioningViewModel
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController

class MainActivity : ComponentActivity() {

    private lateinit var rxBleClient: RxBleClient
    private lateinit var bleProvisioningClient: BleProvisioningClient
    private lateinit var cloudStore: FirebaseCloudStore
    private lateinit var deviceRepository: DeviceRepository

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        rxBleClient = RxBleClient.create(this)
        bleProvisioningClient = BleProvisioningClient(rxBleClient)
        cloudStore = FirebaseCloudStore()
        deviceRepository = DeviceRepository(cloudStore)

        setContent {
            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    AppNavigation(bleProvisioningClient, deviceRepository, cloudStore)
                }
            }
        }
    }
}

@Composable
fun AppNavigation(
    bleProvisioningClient: BleProvisioningClient,
    deviceRepository: DeviceRepository,
    cloudStore: FirebaseCloudStore
) {
    val navController = rememberNavController()
    val auth = FirebaseAuth.getInstance()
    val startDestination = if (auth.currentUser != null) "device_list" else "login"

    NavHost(navController = navController, startDestination = startDestination) {
        composable("login") {
            LoginScreen(
                onLoginSuccess = {
                    navController.navigate("device_list") {
                        popUpTo("login") { inclusive = true }
                    }
                }
            )
        }
        
        composable("device_list") {
            // Mock list for now, ideally fetched from a user's claimed devices list
            val claimedDevices = listOf("SmartFlow-123", "SmartFlow-456")
            DeviceListScreen(
                devices = claimedDevices,
                onDeviceSelected = { deviceId ->
                    navController.navigate("dashboard/$deviceId")
                },
                onAddNewDevice = {
                    navController.navigate("provisioning")
                }
            )
        }
        
        composable("provisioning") {
            val viewModel = ProvisioningViewModel(bleProvisioningClient, cloudStore)
            ProvisioningScreen(
                viewModel = viewModel,
                onProvisioningSuccess = {
                    navController.navigate("device_list") {
                        popUpTo("device_list") { inclusive = true }
                    }
                }
            )
        }
        
        composable("dashboard/{deviceId}") { backStackEntry ->
            val deviceId = backStackEntry.arguments?.getString("deviceId") ?: ""
            val firebaseRepo = com.smartflow.data.repository.FirebaseDeviceRepository(deviceId)
            val viewModel = DashboardViewModel(firebaseRepo)
            DashboardScreen(viewModel = viewModel)
        }
    }
}
