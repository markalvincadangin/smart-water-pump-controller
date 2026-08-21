package com.smartflow.presentation.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.domain.DeviceConfig
import com.smartflow.presentation.components.settings.ThresholdControl
import com.smartflow.presentation.components.settings.MaintenanceOverrideRow
import com.smartflow.presentation.components.dialogs.ConfirmationDialog
import com.smartflow.presentation.theme.CyanPrimary

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ConfigBottomSheet(
    currentConfig: DeviceConfig,
    bypassLevel: Boolean,
    bypassFlow: Boolean,
    onConfigChanged: (DeviceConfig) -> Unit,
    onBypassChanged: (Boolean, Boolean) -> Unit,
    onReboot: () -> Unit,
    onDismissRequest: () -> Unit,
    sheetState: SheetState = rememberModalBottomSheetState(skipPartiallyExpanded = true)
) {
    var lowLevel by remember { mutableFloatStateOf(currentConfig.lowLevelThreshold.toFloat()) }
    var dryRun by remember { mutableFloatStateOf(currentConfig.dryRunThresholdLmin) }
    var maxOverflow by remember { mutableFloatStateOf(currentConfig.maxOverflowTimeoutMins.toFloat()) }
    var localBypassLevel by remember { mutableStateOf(bypassLevel) }
    var localBypassFlow by remember { mutableStateOf(bypassFlow) }

    var lowLevelError by remember { mutableStateOf(false) }
    var dryRunError by remember { mutableStateOf(false) }
    var maxOverflowError by remember { mutableStateOf(false) }
    val hasAnyError = lowLevelError || dryRunError || maxOverflowError

    var showLevelBypassConfirm by remember { mutableStateOf(false) }
    var showFlowBypassConfirm by remember { mutableStateOf(false) }

    if (showLevelBypassConfirm) {
        ConfirmationDialog(
            title = "Bypass level sensor?",
            text = "Automatic low-water protection that depends on the level sensor will be unavailable.",
            confirmText = "Enable bypass",
            onConfirm = {
                localBypassLevel = true
                showLevelBypassConfirm = false
            },
            onDismiss = { showLevelBypassConfirm = false }
        )
    }

    if (showFlowBypassConfirm) {
        ConfirmationDialog(
            title = "Bypass flow sensor?",
            text = "Automatic dry-run protection that depends on the flow sensor will be unavailable.",
            confirmText = "Enable bypass",
            onConfirm = {
                localBypassFlow = true
                showFlowBypassConfirm = false
            },
            onDismiss = { showFlowBypassConfirm = false }
        )
    }

    ModalBottomSheet(
        onDismissRequest = onDismissRequest,
        sheetState = sheetState,
        containerColor = MaterialTheme.colorScheme.surfaceVariant
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(24.dp)
                .padding(bottom = 32.dp),
            verticalArrangement = Arrangement.spacedBy(24.dp)
        ) {
            Text(
                text = "Device Configuration",
                style = MaterialTheme.typography.headlineMedium,
                color = MaterialTheme.colorScheme.onSurface
            )

            // Low Level Threshold
            ThresholdControl(
                title = "Low Water Threshold",
                value = lowLevel,
                onValueChange = { lowLevel = it },
                valueRange = 0f..50f,
                steps = 50,
                unit = "%",
                description = "Stops the pump when tank level falls below this threshold.",
                onErrorChange = { lowLevelError = it }
            )

            // Dry Run Threshold
            ThresholdControl(
                title = "Dry-Run Threshold",
                value = dryRun,
                onValueChange = { dryRun = it },
                valueRange = 0f..5f,
                steps = 50,
                unit = "L/min",
                description = "Stops the pump if flow remains below this threshold.",
                onErrorChange = { dryRunError = it }
            )

            // Max Overflow Timeout
            ThresholdControl(
                title = "Max Continuous Run",
                value = maxOverflow,
                onValueChange = { maxOverflow = it },
                valueRange = 5f..60f,
                steps = 55,
                unit = "mins",
                description = "Safety timeout to prevent continuous overflow.",
                onErrorChange = { maxOverflowError = it }
            )

            HorizontalDivider(color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.1f))

            Text(
                text = "Maintenance Overrides",
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.onSurface
            )

            Text(
                text = "Use only during maintenance.",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )

            MaintenanceOverrideRow(
                title = "Bypass level sensor",
                description = "Disables level-based pump protection.",
                checked = localBypassLevel,
                onEnableClick = { showLevelBypassConfirm = true },
                onDisableClick = { localBypassLevel = false }
            )

            MaintenanceOverrideRow(
                title = "Bypass flow sensor",
                description = "Disables flow-based dry run protection.",
                checked = localBypassFlow,
                onEnableClick = { showFlowBypassConfirm = true },
                onDisableClick = { localBypassFlow = false }
            )

            HorizontalDivider(color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.1f))

            Text(
                text = "Device maintenance",
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.onSurface
            )

            var showRebootConfirm by remember { mutableStateOf(false) }
            var isRebooting by remember { mutableStateOf(false) }

            if (showRebootConfirm) {
                ConfirmationDialog(
                    title = "Confirm Reboot",
                    text = "Are you sure you want to reboot the ESP32? The device will go offline for a moment.",
                    confirmText = "Reboot",
                    onConfirm = {
                        showRebootConfirm = false
                        isRebooting = true
                        onReboot()
                    },
                    onDismiss = { showRebootConfirm = false }
                )
            }

            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(MaterialTheme.colorScheme.surfaceVariant)
                    .padding(vertical = 8.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text(
                    text = "Reboot device",
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.onSurface
                )
                Text(
                    text = "Restarts the controller. Pump availability may be temporarily interrupted.",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                OutlinedButton(
                    onClick = { showRebootConfirm = true },
                    enabled = !isRebooting,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    if (isRebooting) {
                        CircularProgressIndicator(
                            modifier = Modifier.size(24.dp),
                            color = MaterialTheme.colorScheme.primary,
                            strokeWidth = 2.dp
                        )
                    } else {
                        Text("Reboot device")
                    }
                }
            }

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.End
            ) {
                TextButton(onClick = onDismissRequest) {
                    Text("CANCEL", color = MaterialTheme.colorScheme.onSurface)
                }
                Spacer(modifier = Modifier.width(8.dp))
                Button(
                    onClick = {
                        onConfigChanged(
                            DeviceConfig(
                                lowLevelThreshold = lowLevel.toInt(),
                                dryRunThresholdLmin = dryRun,
                                maxOverflowTimeoutMins = maxOverflow.toInt()
                            )
                        )
                        onBypassChanged(localBypassLevel, localBypassFlow)
                        onDismissRequest()
                    },
                    enabled = !hasAnyError,
                    colors = ButtonDefaults.buttonColors(containerColor = CyanPrimary)
                ) {
                    Text("SAVE CHANGES", fontWeight = FontWeight.Bold)
                }
            }
        }
    }
}
