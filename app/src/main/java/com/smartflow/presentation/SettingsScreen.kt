package com.smartflow.presentation

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.ui.theme.ThemePreference
import com.smartflow.viewmodel.SettingsViewModel
import com.smartflow.presentation.components.SmartFlowTopAppBar
import com.smartflow.ui.theme.LocalSpacing

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    viewModel: SettingsViewModel,
    onBack: () -> Unit,
    onManageAccount: () -> Unit,
    onNotificationSettings: () -> Unit
) {
    val themePreference by viewModel.themePreference.collectAsState()
    val spacing = LocalSpacing.current

    Scaffold(
        topBar = {
            SmartFlowTopAppBar(
                title = "Settings",
                showBackButton = true,
                onBackClick = onBack
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
        ) {
            ListItem(
                headlineContent = { Text("Account Management", fontWeight = FontWeight.Bold) },
                supportingContent = { Text("Manage email, password, and sign out") },
                modifier = Modifier.clickable { onManageAccount() }
            )
            
            ListItem(
                headlineContent = { Text("Notification Settings", fontWeight = FontWeight.Bold) },
                supportingContent = { Text("Configure push alerts and warnings") },
                modifier = Modifier.clickable { onNotificationSettings() }
            )
            
            HorizontalDivider()

            Text(
                text = "App Theme",
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colorScheme.primary,
                modifier = Modifier.padding(spacing.medium)
            )

            ThemeSelectionItem(
                label = "System Default",
                isSelected = themePreference == ThemePreference.SYSTEM_DEFAULT,
                onClick = { viewModel.setThemePreference(ThemePreference.SYSTEM_DEFAULT) }
            )
            ThemeSelectionItem(
                label = "Light",
                isSelected = themePreference == ThemePreference.LIGHT,
                onClick = { viewModel.setThemePreference(ThemePreference.LIGHT) }
            )
            ThemeSelectionItem(
                label = "Dark",
                isSelected = themePreference == ThemePreference.DARK,
                onClick = { viewModel.setThemePreference(ThemePreference.DARK) }
            )
            ThemeSelectionItem(
                label = "Monochrome (White)",
                isSelected = themePreference == ThemePreference.MONOCHROME_LIGHT,
                onClick = { viewModel.setThemePreference(ThemePreference.MONOCHROME_LIGHT) }
            )
            ThemeSelectionItem(
                label = "Monochrome (Black)",
                isSelected = themePreference == ThemePreference.MONOCHROME_DARK,
                onClick = { viewModel.setThemePreference(ThemePreference.MONOCHROME_DARK) }
            )
        }
    }
}

@Composable
private fun ThemeSelectionItem(
    label: String,
    isSelected: Boolean,
    onClick: () -> Unit
) {
    val spacing = LocalSpacing.current
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clickable(onClick = onClick)
            .padding(horizontal = spacing.medium, vertical = spacing.mediumSmall),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = label,
            modifier = Modifier.weight(1f),
            style = MaterialTheme.typography.bodyLarge
        )
        RadioButton(
            selected = isSelected,
            onClick = null
        )
    }
}
