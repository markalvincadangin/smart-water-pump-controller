package com.smartflow

import android.os.Bundle
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
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
import com.smartflow.data.AccountSession
import com.smartflow.data.DurableAccountState
import com.smartflow.presentation.DashboardScreen
import com.smartflow.presentation.DeviceListScreen
import com.smartflow.presentation.DeviceOwnershipScreen
import com.smartflow.presentation.AccountManagementScreen
import com.smartflow.presentation.LoginScreen
import com.smartflow.presentation.ProvisioningScreen
import com.smartflow.viewmodel.DashboardViewModel
import com.smartflow.viewmodel.ProvisioningViewModel
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue

private fun hasEligibleAccount(): Boolean {
    return AccountSession.state(FirebaseAuth.getInstance().currentUser) == DurableAccountState.ELIGIBLE
}

class MainActivity : ComponentActivity() {

    private lateinit var rxBleClient: RxBleClient
    private lateinit var bleProvisioningClient: BleProvisioningClient
    private lateinit var cloudStore: FirebaseCloudStore
    private lateinit var deviceRepository: DeviceRepository

    private val requestPermissionLauncher = registerForActivityResult(
        androidx.activity.result.contract.ActivityResultContracts.RequestPermission()
    ) { isGranted: Boolean ->
        if (isGranted) {
            // FCM SDK (and your app) can post notifications.
        }
    }

    private fun askNotificationPermission() {
        // This is only necessary for API level >= 33 (TIRAMISU)
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.TIRAMISU) {
            if (androidx.core.content.ContextCompat.checkSelfPermission(this, android.Manifest.permission.POST_NOTIFICATIONS) ==
                android.content.pm.PackageManager.PERMISSION_GRANTED
            ) {
                // FCM SDK (and your app) can post notifications.
            } else if (shouldShowRequestPermissionRationale(android.Manifest.permission.POST_NOTIFICATIONS)) {
                // TODO: display an educational UI explaining to the user the features that will be enabled
                requestPermissionLauncher.launch(android.Manifest.permission.POST_NOTIFICATIONS)
            } else {
                // Directly ask for the permission
                requestPermissionLauncher.launch(android.Manifest.permission.POST_NOTIFICATIONS)
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        installSplashScreen()
        super.onCreate(savedInstanceState)
        
        rxBleClient = RxBleClient.create(this)
        bleProvisioningClient = BleProvisioningClient(rxBleClient)
        cloudStore = FirebaseCloudStore()
        deviceRepository = DeviceRepository(cloudStore)

        askNotificationPermission()
        
        com.google.firebase.messaging.FirebaseMessaging.getInstance().token.addOnCompleteListener { task ->
            if (!task.isSuccessful) {
                android.util.Log.w("FCM", "Fetching FCM registration token failed", task.exception)
                return@addOnCompleteListener
            }
            val token = task.result
            android.util.Log.d("FCM", "FCM token: $token")
            val user = FirebaseAuth.getInstance().currentUser
            if (user != null) {
                val db = com.google.firebase.database.FirebaseDatabase.getInstance()
                db.getReference("users/${user.uid}/notification_prefs/fcmTokens/${token.hashCode()}").setValue(token)
                db.getReference("users/${user.uid}/notification_prefs/enabled").setValue(true)

                db.getReference("users/${user.uid}/devices").get().addOnSuccessListener { snapshot ->
                    for (child in snapshot.children) {
                        if (child.getValue(Boolean::class.java) == true) {
                            val deviceId = child.key
                            if (deviceId != null) {
                                db.getReference("devices/$deviceId/fcmTokens/${token.hashCode()}").setValue(token)
                            }
                        }
                    }
                }
            }
        }

        setContent {
            com.smartflow.ui.theme.SmartFlowTheme {
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
    val startDestination = if (hasEligibleAccount()) "device_list" else "login"

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
            var sessionValidated by remember { mutableStateOf(false) }
            var uid by remember { mutableStateOf<String?>(null) }

            LaunchedEffect(Unit) {
                if (AccountSession.refreshDurableState(auth) == DurableAccountState.ELIGIBLE) {
                    uid = auth.currentUser?.uid
                    sessionValidated = true
                } else {
                    navController.navigate("login") {
                        popUpTo("device_list") { inclusive = true }
                    }
                }
            }

            if (!sessionValidated) {
                androidx.compose.foundation.layout.Box(
                    modifier = Modifier.fillMaxSize(),
                    contentAlignment = androidx.compose.ui.Alignment.Center
                ) { androidx.compose.material3.CircularProgressIndicator() }
            } else if (uid != null) {
                val authenticatedUid = requireNotNull(uid)
                val devicesFlow = deviceRepository.getUserDevicesStream(authenticatedUid)
                val claimedDevices by devicesFlow.collectAsState(initial = null)

                LaunchedEffect(claimedDevices) {
                    if (claimedDevices != null && claimedDevices!!.isEmpty()) {
                        navController.navigate("provisioning") {
                            popUpTo("device_list") { inclusive = true }
                        }
                    }
                }

                if (claimedDevices == null) {
                    // Loading state
                    androidx.compose.foundation.layout.Box(
                        modifier = Modifier.fillMaxSize(),
                        contentAlignment = androidx.compose.ui.Alignment.Center
                    ) {
                        androidx.compose.material3.CircularProgressIndicator()
                    }
                } else if (claimedDevices!!.isNotEmpty()) {
                    DeviceListScreen(
                        devices = claimedDevices!!,
                        onDeviceSelected = { deviceId ->
                            navController.navigate("dashboard/$deviceId")
                        },
                        onAddNewDevice = {
                            navController.navigate("provisioning")
                        },
                        onManageOwnership = { deviceId -> navController.navigate("device_ownership/$deviceId") },
                        onManageAccount = { navController.navigate("account") },
                    )
                }
            } else {
                // If uid is null, fallback to login
                LaunchedEffect(Unit) {
                    navController.navigate("login") {
                        popUpTo("device_list") { inclusive = true }
                    }
                }
            }
        }
        
        composable("provisioning") {
            val viewModel = ProvisioningViewModel(bleProvisioningClient, cloudStore)
            ProvisioningScreen(
                viewModel = viewModel,
                onProvisioningSuccess = {
                    navController.navigate("device_list") {
                        popUpTo("device_list") { inclusive = true }
                    }
                },
                onBack = { navController.popBackStack() }
            )
        }
        
        composable("dashboard/{deviceId}") { backStackEntry ->
            val deviceId = backStackEntry.arguments?.getString("deviceId") ?: ""
            val firebaseRepo = com.smartflow.data.repository.FirebaseDeviceRepository(deviceId)
            val viewModel = DashboardViewModel(firebaseRepo)
            DashboardScreen(
                viewModel = viewModel,
                onBack = { navController.popBackStack() }
            )
        }

        composable("device_ownership/{deviceId}") { backStackEntry ->
            val deviceId = backStackEntry.arguments?.getString("deviceId") ?: return@composable
            DeviceOwnershipScreen(
                deviceId = deviceId,
                onStartTransfer = { recipientUid -> cloudStore.startOwnershipTransfer(deviceId, recipientUid) },
                onRelease = { cloudStore.releaseDevice(deviceId) },
                onCancelPairing = { cloudStore.cancelOwnershipPairing(deviceId) },
                onRequestWifiRecovery = { cloudStore.requestWifiReprovision(deviceId) },
                onOpenProvisioning = {
                    navController.navigate("provisioning") {
                        popUpTo("device_ownership/$deviceId") { inclusive = true }
                    }
                },
                onBack = { navController.popBackStack() },
            )
        }

        composable("account") {
            AccountManagementScreen(
                accountLabel = auth.currentUser?.email ?: auth.currentUser?.uid.orEmpty(),
                onCheckDeletionEligibility = { cloudStore.checkAccountDeletionEligibility() },
                onSignOut = {
                    auth.signOut()
                    navController.navigate("login") {
                        popUpTo(0) { inclusive = true }
                    }
                },
                onBack = { navController.popBackStack() },
            )
        }
    }
}
