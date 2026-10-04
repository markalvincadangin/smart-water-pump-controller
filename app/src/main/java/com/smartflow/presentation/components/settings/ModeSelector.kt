package com.smartflow.presentation.components.settings

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.smartflow.domain.CommandState
import com.smartflow.domain.OperatingMode
import com.smartflow.domain.PendingCommandType
import com.smartflow.ui.theme.LocalSpacing

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ModeSelector(
    operatingMode: OperatingMode,
    desiredMode: OperatingMode,
    commandState: CommandState,
    pendingCommandType: PendingCommandType?,
    onModeSelected: (OperatingMode) -> Unit,
    modifier: Modifier = Modifier
) {
    val spacing = LocalSpacing.current
    val isPendingModeSwitch = pendingCommandType == PendingCommandType.MODE_CHANGE && operatingMode != desiredMode
    val isCommandPending = pendingCommandType != null

    Column(modifier = modifier.fillMaxWidth()) {
        SingleChoiceSegmentedButtonRow(
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = spacing.small)
        ) {
            listOf(OperatingMode.MANUAL, OperatingMode.COUNTDOWN).forEachIndexed { index, mode ->
                SegmentedButton(
                    selected = operatingMode == mode,
                    enabled = !isCommandPending,
                    onClick = { onModeSelected(mode) },
                    shape = SegmentedButtonDefaults.itemShape(index = index, count = 2)
                ) {
                    Text(if (mode == OperatingMode.COUNTDOWN) "Countdown" else "Manual")
                }
            }
        }

        if (operatingMode == OperatingMode.AUTO || desiredMode == OperatingMode.AUTO) {
            Text(
                text = "Auto mode is not available in the current MVP.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.padding(horizontal = spacing.small, vertical = spacing.extraSmall)
            )
        } else if (isPendingModeSwitch) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier.padding(horizontal = spacing.small, vertical = spacing.extraSmall)
            ) {
                if (commandState is CommandState.Pending || commandState is CommandState.Accepted) {
                    CircularProgressIndicator(
                        modifier = Modifier.size(12.dp),
                        strokeWidth = 2.dp,
                        color = MaterialTheme.colorScheme.primary
                    )
                    Spacer(modifier = Modifier.width(spacing.small))
                    val friendlyName = desiredMode.name.lowercase().replaceFirstChar { it.uppercase() }
                    Text(
                        text = "Switching to $friendlyName...",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.primary
                    )
                } else if (commandState is CommandState.Rejected) {
                    Text(
                        text = "Mode change rejected: ${commandState.reason}",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.error
                    )
                }
            }
        }
    }
}
