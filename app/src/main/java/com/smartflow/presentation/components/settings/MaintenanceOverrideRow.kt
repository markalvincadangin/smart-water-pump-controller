package com.smartflow.presentation.components.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import com.smartflow.ui.theme.LocalSpacing
import com.smartflow.ui.theme.WarningAmber
import com.smartflow.ui.theme.OnWarning

@Composable
fun MaintenanceOverrideRow(
    title: String,
    description: String,
    checked: Boolean,
    onEnableClick: () -> Unit,
    onDisableClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val spacing = LocalSpacing.current

    if (checked) {
        Column(
            modifier = modifier
                .fillMaxWidth()
                .background(WarningAmber.copy(alpha = 0.2f))
                .padding(spacing.medium),
            verticalArrangement = Arrangement.spacedBy(spacing.small)
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    text = "⚠ ",
                    style = MaterialTheme.typography.titleMedium,
                    color = WarningAmber
                )
                Text(
                    text = "${title.uppercase()} BYPASSED",
                    style = MaterialTheme.typography.titleMedium,
                    color = WarningAmber,
                    fontWeight = FontWeight.Bold
                )
            }
            Text(
                text = "Protection is reduced. $description",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurface
            )
            Button(
                onClick = onDisableClick,
                colors = ButtonDefaults.buttonColors(
                    containerColor = WarningAmber,
                    contentColor = OnWarning
                ),
                modifier = Modifier.align(Alignment.End)
            ) {
                Text("Disable bypass")
            }
        }
    } else {
        Column(
            modifier = modifier
                .fillMaxWidth()
                .background(MaterialTheme.colorScheme.surfaceVariant)
                .padding(spacing.medium),
            verticalArrangement = Arrangement.spacedBy(spacing.small)
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    text = "⚠ ",
                    style = MaterialTheme.typography.titleMedium,
                    color = MaterialTheme.colorScheme.error
                )
                Text(
                    text = title,
                    style = MaterialTheme.typography.titleMedium,
                    color = MaterialTheme.colorScheme.error
                )
            }
            Text(
                text = description,
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            OutlinedButton(
                onClick = onEnableClick,
                colors = ButtonDefaults.outlinedButtonColors(
                    contentColor = MaterialTheme.colorScheme.error
                ),
                modifier = Modifier.align(Alignment.End)
            ) {
                Text("Enable bypass")
            }
        }
    }
}
