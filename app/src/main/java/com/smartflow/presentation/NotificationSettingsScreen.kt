package com.smartflow.presentation

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.KeyboardArrowLeft
import androidx.compose.material.icons.filled.KeyboardArrowRight
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import com.smartflow.presentation.components.SmartFlowTopAppBar
import com.smartflow.ui.theme.LocalSpacing
import com.smartflow.viewmodel.NotificationSettingsViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun NotificationSettingsScreen(
    viewModel: NotificationSettingsViewModel,
    onBack: () -> Unit
) {
    val prefs by viewModel.prefs.collectAsState()
    var showTimeDialog by remember { mutableStateOf(false) }
    val spacing = LocalSpacing.current
    val context = LocalContext.current

    val notificationsGranted = Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU ||
        ContextCompat.checkSelfPermission(
            context,
            Manifest.permission.POST_NOTIFICATIONS
        ) == PackageManager.PERMISSION_GRANTED

    Scaffold(
        topBar = {
            SmartFlowTopAppBar(
                title = "Notification Settings",
                showBackButton = true,
                onBackClick = onBack
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .padding(spacing.medium),
            verticalArrangement = Arrangement.spacedBy(spacing.medium)
        ) {
            NotificationSectionLabel("Notifications", spacing)

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(spacing.medium),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Column(modifier = Modifier.weight(1f)) {
                        Text(
                            "Enable Push Notifications",
                            style = MaterialTheme.typography.titleMedium
                        )
                        Spacer(modifier = Modifier.height(spacing.small))
                        Text(
                            "Controls SmartFlow push delivery. Informational alerts can be configured below.",
                            style = MaterialTheme.typography.bodyMedium
                        )
                    }
                    Spacer(modifier = Modifier.width(spacing.medium))
                    Switch(
                        checked = prefs.enabled,
                        onCheckedChange = { viewModel.updatePrefs(prefs.copy(enabled = it)) }
                    )
                }
            }

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(spacing.medium),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Column(modifier = Modifier.weight(1f)) {
                        Text(
                            "Android Notification Permission",
                            style = MaterialTheme.typography.titleMedium
                        )
                        Spacer(modifier = Modifier.height(spacing.small))
                        Text(
                            if (notificationsGranted) {
                                "Allowed by Android. SmartFlow can show notifications."
                            } else {
                                "Blocked by Android. Enable notifications in system settings to receive alerts."
                            },
                            style = MaterialTheme.typography.bodyMedium
                        )
                    }
                    if (!notificationsGranted) {
                        Spacer(modifier = Modifier.width(spacing.medium))
                        TextButton(
                            onClick = {
                                context.startActivity(
                                    Intent(
                                        Settings.ACTION_APP_NOTIFICATION_SETTINGS,
                                        Uri.parse("package:" + context.packageName)
                                    )
                                )
                            }
                        ) {
                            Text("Open Settings")
                        }
                    }
                }
            }

            NotificationSectionLabel("Quiet Hours", spacing)

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Column {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(spacing.medium),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text(
                                "Quiet Hours",
                                style = MaterialTheme.typography.titleMedium
                            )
                            Spacer(modifier = Modifier.height(spacing.small))
                            Text(
                                "Suppress non-critical notifications during the selected hours.",
                                style = MaterialTheme.typography.bodyMedium
                            )
                        }
                        Spacer(modifier = Modifier.width(spacing.medium))
                        Switch(
                            checked = prefs.dndEnabled && prefs.enabled,
                            enabled = prefs.enabled,
                            onCheckedChange = {
                                viewModel.updatePrefs(prefs.copy(dndEnabled = it))
                            }
                        )
                    }

                    Text(
                        "Pump Started and Low Tank alerts are silenced during Quiet Hours. Dry-Run Lockout and Maximum Runtime Protection remain critical and bypass Quiet Hours.",
                        modifier = Modifier.padding(
                            start = spacing.medium,
                            end = spacing.medium,
                            bottom = spacing.medium
                        ),
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )

                    if (prefs.dndEnabled && prefs.enabled) {
                        HorizontalDivider()
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .clickable { showTimeDialog = true }
                                .padding(spacing.medium),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Column(modifier = Modifier.weight(1f)) {
                                Text(
                                    "Schedule",
                                    style = MaterialTheme.typography.bodyLarge
                                )
                                Text(
                                    "Tap to change quiet hours",
                                    style = MaterialTheme.typography.bodySmall
                                )
                            }
                            Spacer(modifier = Modifier.width(spacing.medium))
                            Text(
                                String.format(
                                    "%02d:00 to %02d:00",
                                    prefs.dndStartHour,
                                    prefs.dndEndHour
                                ),
                                style = MaterialTheme.typography.titleMedium,
                                color = MaterialTheme.colorScheme.primary
                            )
                        }
                    }
                }
            }

            NotificationSectionLabel("Informational Alerts", spacing)

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Column {
                    NotificationPreferenceRow(
                        title = "Pump Started",
                        description = "Receive a notification when the pump starts running.",
                        checked = prefs.pumpStartedAlert && prefs.enabled,
                        enabled = prefs.enabled,
                        onCheckedChange = {
                            viewModel.updatePrefs(prefs.copy(pumpStartedAlert = it))
                        },
                        spacing = spacing
                    )
                    HorizontalDivider()
                    NotificationPreferenceRow(
                        title = "Low Tank Level",
                        description = "Receive a notification when the water level drops below the threshold.",
                        checked = prefs.lowLevelAlert && prefs.enabled,
                        enabled = prefs.enabled,
                        onCheckedChange = {
                            viewModel.updatePrefs(prefs.copy(lowLevelAlert = it))
                        },
                        spacing = spacing
                    )
                }
            }

            NotificationSectionLabel("Critical Safety Alerts", spacing)

            Card(
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant)
            ) {
                Column {
                    NotificationPreferenceRow(
                        title = "Dry-Run Protection",
                        description = "Always enabled. Critical lockout alerts bypass Quiet Hours.",
                        checked = true,
                        enabled = false,
                        onCheckedChange = {},
                        spacing = spacing
                    )
                    HorizontalDivider()
                    NotificationPreferenceRow(
                        title = "Maximum Runtime Protection",
                        description = "Always enabled. Critical protection alerts bypass Quiet Hours.",
                        checked = true,
                        enabled = false,
                        onCheckedChange = {},
                        spacing = spacing
                    )
                }
            }

            Text(
                "Safety alert controls are intentionally separate from informational alerts. Quiet Hours do not suppress critical dry-run or maximum-runtime protection notifications.",
                modifier = Modifier.padding(horizontal = spacing.small),
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
    }

    if (showTimeDialog) {
        var tempStart by remember { mutableStateOf(prefs.dndStartHour) }
        var tempEnd by remember { mutableStateOf(prefs.dndEndHour) }

        AlertDialog(
            onDismissRequest = { showTimeDialog = false },
            title = { Text("Set Quiet Hours") },
            text = {
                Column(verticalArrangement = Arrangement.spacedBy(spacing.medium)) {
                    QuietHourSelector(
                        label = "Start Time",
                        hour = tempStart,
                        onHourChange = { tempStart = it },
                        spacing = spacing
                    )
                    QuietHourSelector(
                        label = "End Time",
                        hour = tempEnd,
                        onHourChange = { tempEnd = it },
                        spacing = spacing
                    )
                }
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        viewModel.updatePrefs(
                            prefs.copy(
                                dndStartHour = tempStart,
                                dndEndHour = tempEnd
                            )
                        )
                        showTimeDialog = false
                    }
                ) {
                    Text("Save")
                }
            },
            dismissButton = {
                TextButton(onClick = { showTimeDialog = false }) {
                    Text("Cancel")
                }
            }
        )
    }
}

@Composable
private fun NotificationSectionLabel(
    text: String,
    spacing: com.smartflow.ui.theme.Spacing
) {
    Text(
        text = text,
        style = MaterialTheme.typography.labelLarge,
        modifier = Modifier.padding(horizontal = spacing.small)
    )
}

@Composable
private fun NotificationPreferenceRow(
    title: String,
    description: String,
    checked: Boolean,
    enabled: Boolean,
    onCheckedChange: (Boolean) -> Unit,
    spacing: com.smartflow.ui.theme.Spacing
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(spacing.medium),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(title, style = MaterialTheme.typography.titleMedium)
            Spacer(modifier = Modifier.height(spacing.small))
            Text(
                description,
                style = MaterialTheme.typography.bodyMedium,
                color = if (enabled) {
                    MaterialTheme.colorScheme.onSurfaceVariant
                } else {
                    MaterialTheme.colorScheme.onSurfaceVariant
                }
            )
        }
        Spacer(modifier = Modifier.width(spacing.medium))
        Switch(
            checked = checked,
            enabled = enabled,
            onCheckedChange = onCheckedChange
        )
    }
}

@Composable
private fun QuietHourSelector(
    label: String,
    hour: Int,
    onHourChange: (Int) -> Unit,
    spacing: com.smartflow.ui.theme.Spacing
) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(label, modifier = Modifier.weight(1f))
        IconButton(
            onClick = { onHourChange(if (hour == 0) 23 else hour - 1) }
        ) {
            Icon(
                Icons.Default.KeyboardArrowLeft,
                contentDescription = "Decrease $label"
            )
        }
        Text(
            String.format("%02d:00", hour),
            style = MaterialTheme.typography.titleMedium
        )
        IconButton(
            onClick = { onHourChange(if (hour == 23) 0 else hour + 1) }
        ) {
            Icon(
                Icons.Default.KeyboardArrowRight,
                contentDescription = "Increase $label"
            )
        }
    }
}
