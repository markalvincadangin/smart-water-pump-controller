package com.smartflow.presentation.components.core

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.ui.theme.LocalSpacing

enum class StatusChipState {
    Healthy,
    Warning,
    Critical,
    Offline
}

@Composable
fun StatusChip(
    state: StatusChipState,
    label: String,
    icon: ImageVector? = null,
    modifier: Modifier = Modifier
) {
    val spacing = LocalSpacing.current

    val (backgroundColor, contentColor) = when (state) {
        StatusChipState.Healthy -> MaterialTheme.colorScheme.secondary to MaterialTheme.colorScheme.onSecondary
        StatusChipState.Warning -> MaterialTheme.colorScheme.tertiary to MaterialTheme.colorScheme.onTertiary
        StatusChipState.Critical -> MaterialTheme.colorScheme.error to MaterialTheme.colorScheme.onError
        StatusChipState.Offline -> MaterialTheme.colorScheme.surfaceContainerHighest to MaterialTheme.colorScheme.onSurfaceVariant
    }

    Row(
        modifier = modifier
            .clip(RoundedCornerShape(spacing.large))
            .background(backgroundColor)
            .padding(horizontal = spacing.medium, vertical = spacing.extraSmall),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(spacing.extraSmall)
    ) {
        if (icon != null) {
            Icon(
                imageVector = icon,
                contentDescription = null,
                tint = contentColor,
                modifier = Modifier.size(16.dp)
            )
        }
        Text(
            text = label,
            style = MaterialTheme.typography.labelMedium,
            color = contentColor,
            fontWeight = FontWeight.Bold
        )
    }
}
