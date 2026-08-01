package com.smartflow.presentation

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.runtime.collectAsState
import com.smartflow.domain.ConnectionState
import com.smartflow.domain.ControlMode
import com.smartflow.presentation.components.ActivityPanel
import com.smartflow.presentation.components.ConfigBottomSheet
import com.smartflow.presentation.components.ControlPanel
import com.smartflow.presentation.components.DiagnosticsCard
import com.smartflow.presentation.components.PumpStatusCard
import com.smartflow.presentation.components.TankLevelCard
import com.smartflow.presentation.components.SmartFlowTopAppBar
import com.smartflow.viewmodel.DashboardViewModel

import androidx.compose.ui.platform.LocalConfiguration
import android.content.res.Configuration
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DashboardScreen(
    viewModel: DashboardViewModel,
    onBack: () -> Unit,
    modifier: Modifier = Modifier
) {
    val uiState by viewModel.uiState.collectAsState()
    var showConfig by remember { mutableStateOf(false) }
    val configuration = LocalConfiguration.current
    val isLandscape = configuration.orientation == Configuration.ORIENTATION_LANDSCAPE

    val snackbarHostState = com.smartflow.LocalSnackbarHostState.current
    val coroutineScope = rememberCoroutineScope()

    LaunchedEffect(uiState.connectionStatus) {
        when (uiState.connectionStatus) {
            ConnectionState.DISCONNECTED -> {
                showConfig = false
                snackbarHostState.showSnackbar(
                    message = "Device is currently offline.",
                    duration = SnackbarDuration.Short
                )
            }
            ConnectionState.CONNECTED -> {
                snackbarHostState.showSnackbar(
                    message = "Device connected.",
                    duration = SnackbarDuration.Short
                )
            }
            ConnectionState.CONNECTING -> {
                showConfig = false
            }
        }
    }

    Scaffold(
        topBar = {
            SmartFlowTopAppBar(
                showBackButton = true,
                onBackClick = onBack,
                actions = {
                    IconButton(
                        onClick = { 
                            if (uiState.connectionStatus == ConnectionState.CONNECTED) {
                                showConfig = true 
                            } else {
                                coroutineScope.launch {
                                    snackbarHostState.showSnackbar("Cannot configure device while offline.")
                                }
                            }
                        },
                        modifier = Modifier.defaultMinSize(minWidth = 48.dp, minHeight = 48.dp)
                    ) {
                        Icon(Icons.Default.Tune, contentDescription = "Device Configuration")
                    }
                }
            )
        },
        containerColor = MaterialTheme.colorScheme.background,
        modifier = modifier.fillMaxSize()
    ) { paddingValues ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState())
            ) {
                // Offline/Stale state banner
                if (uiState.connectionStatus == ConnectionState.DISCONNECTED) {
                    Box(
                        modifier = Modifier
                            .fillMaxWidth()
                            .background(MaterialTheme.colorScheme.error)
                            .padding(8.dp),
                        contentAlignment = Alignment.Center
                    ) {
                        Text(
                            text = "Device is offline. Data is stale.",
                            color = MaterialTheme.colorScheme.onError,
                            style = MaterialTheme.typography.labelLarge,
                            fontWeight = FontWeight.Bold
                        )
                    }
                }
            
            // Main Content
            if (isLandscape) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(16.dp),
                    horizontalArrangement = Arrangement.spacedBy(16.dp)
                ) {
                    // Left Column
                    Column(
                        modifier = Modifier.weight(1f),
                        verticalArrangement = Arrangement.spacedBy(16.dp)
                    ) {
                        DiagnosticsCard(connectionState = uiState.connectionStatus)
                        TankLevelCard(
                            waterLevelPct = uiState.waterLevelPct,
                            connectionState = uiState.connectionStatus
                        )
                        PumpStatusCard(
                            isPumpRunning = uiState.isPumpRunning,
                            flowRateLpm = uiState.flowRateLpm,
                            countdownRemainingSec = uiState.countdownRemainingSec,
                            connectionState = uiState.connectionStatus
                        )
                    }
                    // Right Column
                    Column(
                        modifier = Modifier.weight(1f),
                        verticalArrangement = Arrangement.spacedBy(16.dp)
                    ) {
                        ControlPanel(
                            mode = uiState.mode,
                            desiredMode = uiState.desiredMode,
                            isPumpRunning = uiState.isPumpRunning,
                            lockoutActive = uiState.lockoutActive,
                            connectionState = uiState.connectionStatus,
                            isManualDesired = uiState.isManualDesired,
                            isCountdownStartDesired = uiState.isCountdownStartDesired,
                            lastFaultMessage = uiState.lastFaultMessage,
                            onModeChanged = { viewModel.setControlMode(it) },
                            onEmergencyStop = viewModel::triggerEmergencyStop,
                            onPowerToggle = viewModel::setPumpPower,
                            onCountdownStart = viewModel::startCountdown,
                            onClearError = viewModel::clearErrors
                        )
                        ActivityPanel(events = uiState.events)
                    }
                }
            } else {
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(16.dp)
                ) {
                    DiagnosticsCard(connectionState = uiState.connectionStatus)

                    TankLevelCard(
                        waterLevelPct = uiState.waterLevelPct,
                        connectionState = uiState.connectionStatus
                    )

                    PumpStatusCard(
                        isPumpRunning = uiState.isPumpRunning,
                        flowRateLpm = uiState.flowRateLpm,
                        countdownRemainingSec = uiState.countdownRemainingSec,
                        connectionState = uiState.connectionStatus
                    )

                    ControlPanel(
                        mode = uiState.mode,
                        desiredMode = uiState.desiredMode,
                        isPumpRunning = uiState.isPumpRunning,
                        lockoutActive = uiState.lockoutActive,
                        connectionState = uiState.connectionStatus,
                        isManualDesired = uiState.isManualDesired,
                        isCountdownStartDesired = uiState.isCountdownStartDesired,
                        lastFaultMessage = uiState.lastFaultMessage,
                        onModeChanged = { viewModel.setControlMode(it) },
                        onEmergencyStop = viewModel::triggerEmergencyStop,
                        onPowerToggle = viewModel::setPumpPower,
                        onCountdownStart = viewModel::startCountdown,
                        onClearError = viewModel::clearErrors
                    )

                    ActivityPanel(events = uiState.events)
                }
            }
            } // end Column

            if (uiState.connectionStatus == ConnectionState.CONNECTING) {
                ConnectingOverlay()
            }
        } // end Box

        if (showConfig) {
            ConfigBottomSheet(
                currentConfig = uiState.config,
                bypassLevel = uiState.bypassLevelSensor,
                bypassFlow = uiState.bypassFlowSensor,
                onConfigChanged = viewModel::updateConfig,
                onBypassChanged = viewModel::updateBypass,
                onReboot = viewModel::rebootDevice,
                onDismissRequest = { showConfig = false }
            )
        }
    }
}

@Composable
fun ConnectingOverlay(modifier: Modifier = Modifier) {
    Box(
        modifier = modifier
            .fillMaxSize()
            .background(MaterialTheme.colorScheme.background)
            // Block touches from passing through to underlying UI
            .clickable(
                interactionSource = remember { MutableInteractionSource() },
                indication = null,
                onClick = {}
            ),
        contentAlignment = Alignment.Center
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            CircularProgressIndicator(color = MaterialTheme.colorScheme.primary)
            Text(
                text = "Connecting to SmartFlow...",
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.onBackground
            )
        }
    }
}
