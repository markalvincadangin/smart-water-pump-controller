package com.smartflow.presentation

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.runtime.collectAsState
import com.smartflow.domain.ConnectionState
import com.smartflow.presentation.components.*

import com.smartflow.viewmodel.DashboardViewModel

import androidx.compose.ui.platform.LocalConfiguration
import android.content.res.Configuration

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DashboardScreen(
    viewModel: DashboardViewModel,
    modifier: Modifier = Modifier
) {
    val uiState by viewModel.uiState.collectAsState()
    var showConfig by remember { mutableStateOf(false) }
    val configuration = LocalConfiguration.current
    val isLandscape = configuration.orientation == Configuration.ORIENTATION_LANDSCAPE

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("SmartFlow", fontWeight = FontWeight.Bold) },
                actions = {
                    IconButton(
                        onClick = { showConfig = true },
                        modifier = Modifier.defaultMinSize(minWidth = 48.dp, minHeight = 48.dp)
                    ) {
                        Icon(Icons.Default.Settings, contentDescription = "Settings")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.surface,
                    titleContentColor = MaterialTheme.colorScheme.onSurface,
                    actionIconContentColor = MaterialTheme.colorScheme.onSurface
                )
            )
        },
        containerColor = MaterialTheme.colorScheme.background,
        modifier = modifier.fillMaxSize()
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .verticalScroll(rememberScrollState())
        ) {
            // Offline/Stale state banner
            if (uiState.connectionStatus != ConnectionState.CONNECTED) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(
                            if (uiState.connectionStatus == ConnectionState.CONNECTING) MaterialTheme.colorScheme.tertiary else MaterialTheme.colorScheme.error
                        )
                        .padding(8.dp),
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = if (uiState.connectionStatus == ConnectionState.CONNECTING) 
                                "Connecting to device..." 
                               else "Device is offline. Data is stale.",
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
        }

        if (showConfig) {
            ConfigBottomSheet(
                currentConfig = uiState.config,
                bypassLevel = uiState.bypassLevelSensor,
                bypassFlow = uiState.bypassFlowSensor,
                onConfigChanged = viewModel::updateConfig,
                onBypassChanged = viewModel::updateBypass,
                onDismissRequest = { showConfig = false }
            )
        }
    }
}
