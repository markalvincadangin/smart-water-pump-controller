package com.smartflow.presentation

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Tune
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
import com.smartflow.domain.OperatingMode
import com.smartflow.domain.CommandState
import com.smartflow.domain.SensorAvailability
import com.smartflow.presentation.components.ActivityPanel
import com.smartflow.presentation.components.ConfigBottomSheet
import com.smartflow.presentation.components.ControlPanel
import com.smartflow.presentation.components.DiagnosticsCard
import com.smartflow.presentation.components.PumpStatusCard
import com.smartflow.presentation.components.TankLevelCard
import com.smartflow.presentation.components.SmartFlowTopAppBar
import com.smartflow.viewmodel.DashboardViewModel

import androidx.compose.ui.platform.LocalConfiguration
import com.smartflow.ui.theme.LocalSpacing
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
    val isExpanded = configuration.screenWidthDp >= 600
    val spacing = LocalSpacing.current

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

    var isPendingModeSwitch by remember { mutableStateOf(false) }
    LaunchedEffect(uiState.operatingMode, uiState.desiredMode) {
        if (uiState.operatingMode != uiState.desiredMode) {
            isPendingModeSwitch = true
        } else if (isPendingModeSwitch && uiState.operatingMode == uiState.desiredMode) {
            isPendingModeSwitch = false
            val friendlyName = uiState.operatingMode.name.lowercase().replaceFirstChar { it.uppercase() }
            snackbarHostState.showSnackbar(
                message = "$friendlyName confirmed.",
                duration = SnackbarDuration.Short
            )
        }
    }

    Scaffold(
        topBar = {
            SmartFlowTopAppBar(
                showBackButton = true,
                onBackClick = onBack,
                actions = {
                    if (uiState.commandState is CommandState.Pending) {
                        CircularProgressIndicator(
                            modifier = Modifier
                                .padding(end = spacing.medium)
                                .size(spacing.large)
                                .align(Alignment.CenterVertically),
                            color = MaterialTheme.colorScheme.primary,
                            strokeWidth = 2.dp
                        )
                    }
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
                        modifier = Modifier.defaultMinSize(minWidth = spacing.doubleExtraLarge, minHeight = spacing.doubleExtraLarge)
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
                // Global Status Banner
                if (uiState.connectionStatus == ConnectionState.DISCONNECTED) {
                    Box(
                        modifier = Modifier
                            .fillMaxWidth()
                            .background(MaterialTheme.colorScheme.error)
                            .padding(spacing.small),
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
            if (isExpanded) {
                // Two column layout for wider screens
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(spacing.medium),
                    horizontalArrangement = Arrangement.spacedBy(spacing.medium)
                ) {
                    // Left Column
                    Column(
                        modifier = Modifier.weight(1f),
                        verticalArrangement = Arrangement.spacedBy(spacing.medium)
                    ) {
                        DiagnosticsCard(connectionState = uiState.connectionStatus)
                        TankLevelCard(waterLevel = uiState.waterLevel)
                        PumpStatusCard(
                            pumpState = uiState.pumpState,
                            operatingMode = uiState.operatingMode,
                            flowRate = uiState.flowRate,
                            connectionState = uiState.connectionStatus
                        )
                    }
                    // Right Column
                    Column(
                        modifier = Modifier.weight(1f),
                        verticalArrangement = Arrangement.spacedBy(spacing.medium)
                    ) {
                        ControlPanel(
                            operatingMode = uiState.operatingMode,
                            desiredMode = uiState.desiredMode,
                            pumpState = uiState.pumpState,
                            connectionState = uiState.connectionStatus,
                            commandState = uiState.commandState,
                            lastFaultMessage = uiState.lastFaultMessage,
                            maxRuntimeLimitMins = uiState.config.maxOverflowTimeoutMins,
                            countdownRemainingSec = uiState.countdownRemainingSec,
                            countdownDurationMin = uiState.countdownDurationMin,
                            onModeChanged = { viewModel.setControlMode(it) },
                            onEmergencyStop = viewModel::triggerEmergencyStop,
                            onPowerToggle = viewModel::setPumpPower,
                            onCountdownStart = viewModel::startCountdown,
                            onCountdownStop = viewModel::stopCountdown,
                            onClearError = viewModel::clearErrors
                        )
                        ActivityPanel(events = uiState.events)
                    }
                }
            } else {
                // Single column layout strictly matching requested hierarchy
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(spacing.medium),
                    verticalArrangement = Arrangement.spacedBy(spacing.medium)
                ) {
                    DiagnosticsCard(connectionState = uiState.connectionStatus)

                    TankLevelCard(waterLevel = uiState.waterLevel)

                    PumpStatusCard(
                        pumpState = uiState.pumpState,
                        operatingMode = uiState.operatingMode,
                        flowRate = uiState.flowRate,
                        connectionState = uiState.connectionStatus
                    )

                    ControlPanel(
                        operatingMode = uiState.operatingMode,
                        desiredMode = uiState.desiredMode,
                        pumpState = uiState.pumpState,
                        connectionState = uiState.connectionStatus,
                        commandState = uiState.commandState,
                        lastFaultMessage = uiState.lastFaultMessage,
                        maxRuntimeLimitMins = uiState.config.maxOverflowTimeoutMins,
                        countdownRemainingSec = uiState.countdownRemainingSec,
                        countdownDurationMin = uiState.countdownDurationMin,
                        onModeChanged = { viewModel.setControlMode(it) },
                        onEmergencyStop = viewModel::triggerEmergencyStop,
                        onPowerToggle = viewModel::setPumpPower,
                        onCountdownStart = viewModel::startCountdown,
                        onCountdownStop = viewModel::stopCountdown,
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
                bypassLevel = uiState.levelSensorAvailability == SensorAvailability.Bypassed,
                bypassFlow = uiState.flowSensorAvailability == SensorAvailability.Bypassed,
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
    val spacing = LocalSpacing.current

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
            verticalArrangement = Arrangement.spacedBy(spacing.medium)
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
