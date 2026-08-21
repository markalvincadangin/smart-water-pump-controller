package com.smartflow.presentation.components.banners

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.rounded.CloudOff
import androidx.compose.material.icons.rounded.Warning
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import com.smartflow.domain.ConnectionState
import com.smartflow.domain.DataFreshness

@Composable
fun ConnectionBanner(
    connectionState: ConnectionState,
    dataFreshness: DataFreshness,
    modifier: Modifier = Modifier
) {
    if (connectionState == ConnectionState.CONNECTED && dataFreshness == DataFreshness.Live) {
        return // Everything is healthy
    }

    val (type, title, message, icon) = when {
        connectionState == ConnectionState.DISCONNECTED -> {
            listOf(
                BannerType.Offline,
                "Device Offline",
                "Local control unavailable. Cloud history may be stale.",
                Icons.Rounded.CloudOff
            )
        }
        dataFreshness == DataFreshness.Stale -> {
            listOf(
                BannerType.Warning,
                "Stale Data",
                "Telemetry hasn't updated recently. State may be inaccurate.",
                Icons.Rounded.Warning
            )
        }
        else -> {
            listOf(
                BannerType.Warning,
                "Connecting...",
                "Attempting to establish connection.",
                Icons.Rounded.Warning
            )
        }
    }

    GlobalStatusBanner(
        type = type as BannerType,
        title = title as String,
        message = message as String,
        icon = icon as androidx.compose.ui.graphics.vector.ImageVector,
        modifier = modifier
    )
}
