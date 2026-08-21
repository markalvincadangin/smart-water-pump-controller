package com.smartflow.presentation.components.banners

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import com.smartflow.ui.theme.LocalSpacing

enum class BannerType {
    Warning,
    Critical,
    Offline
}

@Composable
fun GlobalStatusBanner(
    type: BannerType,
    title: String,
    message: String,
    icon: ImageVector? = null,
    modifier: Modifier = Modifier
) {
    val spacing = LocalSpacing.current

    val (backgroundColor, contentColor) = when (type) {
        BannerType.Warning -> MaterialTheme.colorScheme.tertiary to MaterialTheme.colorScheme.onTertiary
        BannerType.Critical -> MaterialTheme.colorScheme.error to MaterialTheme.colorScheme.onError
        BannerType.Offline -> MaterialTheme.colorScheme.surfaceContainerHighest to MaterialTheme.colorScheme.onSurfaceVariant
    }

    Row(
        modifier = modifier
            .fillMaxWidth()
            .background(backgroundColor)
            .padding(horizontal = spacing.medium, vertical = spacing.small),
        verticalAlignment = Alignment.Top,
        horizontalArrangement = Arrangement.spacedBy(spacing.medium)
    ) {
        if (icon != null) {
            Icon(
                imageVector = icon,
                contentDescription = null,
                tint = contentColor,
                modifier = Modifier.padding(top = spacing.extraSmall)
            )
        }
        Column(
            verticalArrangement = Arrangement.spacedBy(spacing.extraSmall)
        ) {
            Text(
                text = title,
                style = MaterialTheme.typography.titleSmall,
                color = contentColor,
                fontWeight = FontWeight.Bold
            )
            Text(
                text = message,
                style = MaterialTheme.typography.bodyMedium,
                color = contentColor
            )
        }
    }
}
