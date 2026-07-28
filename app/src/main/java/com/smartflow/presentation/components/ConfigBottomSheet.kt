package com.smartflow.presentation.components

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.domain.DeviceConfig
import com.smartflow.presentation.theme.CyanPrimary

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ConfigBottomSheet(
    currentConfig: DeviceConfig,
    bypassLevel: Boolean,
    bypassFlow: Boolean,
    onConfigChanged: (DeviceConfig) -> Unit,
    onBypassChanged: (Boolean, Boolean) -> Unit,
    onDismissRequest: () -> Unit,
    sheetState: SheetState = rememberModalBottomSheetState(skipPartiallyExpanded = true)
) {
    var lowLevel by remember { mutableFloatStateOf(currentConfig.lowLevelThreshold.toFloat()) }
    var dryRun by remember { mutableFloatStateOf(currentConfig.dryRunThresholdLmin) }
    var maxOverflow by remember { mutableFloatStateOf(currentConfig.maxOverflowTimeoutMins.toFloat()) }
    var localBypassLevel by remember { mutableStateOf(bypassLevel) }
    var localBypassFlow by remember { mutableStateOf(bypassFlow) }

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
            ConfigSlider(
                label = "Low Water Threshold (%)",
                value = lowLevel,
                valueRange = 0f..50f,
                steps = 50,
                onValueChange = { lowLevel = it }
            )

            // Dry Run Threshold
            ConfigSlider(
                label = "Dry-Run Threshold (L/min)",
                value = dryRun,
                valueRange = 0f..5f,
                steps = 50,
                onValueChange = { dryRun = it }
            )

            // Max Overflow Timeout
            ConfigSlider(
                label = "Max Continuous Run (mins)",
                value = maxOverflow,
                valueRange = 5f..60f,
                steps = 55,
                onValueChange = { maxOverflow = it }
            )

            HorizontalDivider(color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.1f))

            Text(
                text = "Maintenance Overrides",
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.onSurface
            )

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = androidx.compose.ui.Alignment.CenterVertically
            ) {
                Text("Bypass Level Sensor")
                Switch(
                    checked = localBypassLevel,
                    onCheckedChange = { localBypassLevel = it },
                    colors = SwitchDefaults.colors(checkedThumbColor = CyanPrimary, checkedTrackColor = CyanPrimary.copy(alpha = 0.5f))
                )
            }

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = androidx.compose.ui.Alignment.CenterVertically
            ) {
                Text("Bypass Flow Sensor")
                Switch(
                    checked = localBypassFlow,
                    onCheckedChange = { localBypassFlow = it },
                    colors = SwitchDefaults.colors(checkedThumbColor = CyanPrimary, checkedTrackColor = CyanPrimary.copy(alpha = 0.5f))
                )
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
                    colors = ButtonDefaults.buttonColors(containerColor = CyanPrimary)
                ) {
                    Text("SAVE CHANGES", fontWeight = FontWeight.Bold)
                }
            }
        }
    }
}

@Composable
private fun ConfigSlider(
    label: String,
    value: Float,
    valueRange: ClosedFloatingPointRange<Float>,
    steps: Int,
    onValueChange: (Float) -> Unit
) {
    Column {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(text = label, style = MaterialTheme.typography.bodyMedium)
            Text(
                text = String.format("%.1f", value),
                style = MaterialTheme.typography.labelLarge
            )
        }
        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = valueRange,
            steps = steps,
            colors = SliderDefaults.colors(
                thumbColor = CyanPrimary,
                activeTrackColor = CyanPrimary
            )
        )
    }
}
