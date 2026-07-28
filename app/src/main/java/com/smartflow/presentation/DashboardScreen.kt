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
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.smartflow.domain.ConnectionState
import com.smartflow.presentation.components.*
import com.smartflow.presentation.theme.AmberWarning
import com.smartflow.presentation.theme.DarkSlateSurface
import com.smartflow.presentation.theme.RedError
import com.smartflow.viewmodel.DashboardViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DashboardScreen(
    viewModel: DashboardViewModel,
    modifier: Modifier = Modifier
) {
    val uiState by viewModel.uiState.collectAsStateWithLifecycle()
    var showConfig by remember { mutableStateOf(false) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("SmartFlow", fontWeight = FontWeight.Bold) },
                actions = {
                    IconButton(onClick = { showConfig = true }) {
                        Icon(Icons.Default.Settings, contentDescription = "Settings")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = DarkSlateSurface,
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
                            if (uiState.connectionStatus == ConnectionState.CONNECTING) AmberWarning else RedError
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
                    onModeChanged = viewModel::setControlMode,
                    onEmergencyStop = viewModel::triggerEmergencyStop,
                    onPowerToggle = viewModel::setPumpPower,
                    onCountdownStart = viewModel::startCountdown,
                    onClearError = viewModel::clearErrors
                )

                ActivityPanel()
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
