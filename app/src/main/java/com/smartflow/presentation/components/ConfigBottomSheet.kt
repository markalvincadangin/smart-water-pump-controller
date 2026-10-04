package com.smartflow.presentation.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.domain.DeviceConfig
import com.smartflow.domain.DeviceConfigValidator
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
    var startLevel by remember { mutableFloatStateOf(currentConfig.pumpStartLevelPct.toFloat()) }
    var stopLevel by remember { mutableFloatStateOf(currentConfig.pumpStopLevelPct.toFloat()) }
    var dryRun by remember { mutableFloatStateOf(currentConfig.dryRunThresholdLpm) }
    var maxRuntime by remember { mutableFloatStateOf(currentConfig.maxPumpRuntimeMin.toFloat()) }
    var localBypassLevel by remember { mutableStateOf(bypassLevel) }
    var localBypassFlow by remember { mutableStateOf(bypassFlow) }

    var startLevelError by remember { mutableStateOf(false) }
    var stopLevelError by remember { mutableStateOf(false) }
    var dryRunError by remember { mutableStateOf(false) }
    var maxRuntimeError by remember { mutableStateOf(false) }

    val validation = remember(startLevel, stopLevel, dryRun, maxRuntime) {
        DeviceConfigValidator.validate(
            DeviceConfig(
                pumpStartLevelPct = startLevel.toInt(),
                pumpStopLevelPct = stopLevel.toInt(),
                dryRunThresholdLpm = dryRun,
                maxPumpRuntimeMin = maxRuntime.toInt()
            )
        )
    }
    val hasAnyError = startLevelError || stopLevelError || dryRunError || maxRuntimeError || !validation.isValid

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

            // Pump Start Level
            ThresholdControl(
                title = "Pump Start Level",
                value = startLevel,
                onValueChange = { startLevel = it },
                valueRange = DeviceConfigValidator.PUMP_LEVEL_MIN_PCT.toFloat()..DeviceConfigValidator.PUMP_LEVEL_MAX_PCT.toFloat(),
                steps = 99,
                unit = "%",
                description = "Level threshold used to allow the pump to turn ON.",
                onErrorChange = { startLevelError = it }
            )

            // Pump Stop Level
            ThresholdControl(
                title = "Pump Stop Level",
                value = stopLevel,
                onValueChange = { stopLevel = it },
                valueRange = DeviceConfigValidator.PUMP_LEVEL_MIN_PCT.toFloat()..DeviceConfigValidator.PUMP_LEVEL_MAX_PCT.toFloat(),
                steps = 99,
                unit = "%",
                description = "Level threshold used to turn the pump OFF.",
                onErrorChange = { stopLevelError = it }
            )

            if (validation.pumpStopLevelError != null && !stopLevelError) {
                Text(
                    text = validation.pumpStopLevelError,
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.error
                )
            }

            // Dry Run Threshold
            ThresholdControl(
                title = "Dry-Run Threshold",
                value = dryRun,
                onValueChange = { dryRun = it },
                valueRange = DeviceConfigValidator.DRY_RUN_THRESHOLD_MIN_LPM..DeviceConfigValidator.DRY_RUN_THRESHOLD_MAX_LPM,
                steps = 98,
                unit = "L/min",
                description = "Stops the pump if flow remains below this threshold.",
                onErrorChange = { dryRunError = it }
            )

            // Maximum Pump Runtime
            ThresholdControl(
                title = "Maximum Pump Runtime",
                value = maxRuntime,
                onValueChange = { maxRuntime = it },
                valueRange = DeviceConfigValidator.MAX_PUMP_RUNTIME_MIN_MINUTES.toFloat()..DeviceConfigValidator.MAX_PUMP_RUNTIME_MAX_MINUTES.toFloat(),
                steps = 89,
                unit = "mins",
                description = "Safety timeout that stops the pump after continuous operation exceeds this limit.",
                onErrorChange = { maxRuntimeError = it }
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
                                pumpStartLevelPct = startLevel.toInt(),
                                pumpStopLevelPct = stopLevel.toInt(),
                                dryRunThresholdLpm = dryRun,
                                maxPumpRuntimeMin = maxRuntime.toInt()
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
