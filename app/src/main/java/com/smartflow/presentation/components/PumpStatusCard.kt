package com.smartflow.presentation.components

import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.domain.ConnectionState
import com.smartflow.presentation.theme.EmeraldSecondary
import com.smartflow.presentation.theme.GrayText

@Composable
fun PumpStatusCard(
    isPumpRunning: Boolean,
    flowRateLpm: Float,
    countdownRemainingSec: Int,
    connectionState: ConnectionState,
    modifier: Modifier = Modifier
) {
    val isStale = connectionState != ConnectionState.CONNECTED

    val infiniteTransition = rememberInfiniteTransition()
    val haloScale by infiniteTransition.animateFloat(
        initialValue = 1f,
        targetValue = 1.3f,
        animationSpec = infiniteRepeatable(
            animation = tween(1000, easing = FastOutSlowInEasing),
            repeatMode = RepeatMode.Reverse
        )
    )

    Card(
        modifier = modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(24.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Column {
                Text(
                    text = "Pump Status",
                    style = MaterialTheme.typography.bodyMedium,
                    color = GrayText
                )
                Spacer(modifier = Modifier.height(4.dp))
                Row(verticalAlignment = Alignment.CenterVertically) {
                    val statusText = if (isStale) "UNKNOWN" else if (isPumpRunning) "RUNNING" else "STOPPED"
                    val statusColor = if (isStale) GrayText else if (isPumpRunning) EmeraldSecondary else MaterialTheme.colorScheme.onSurface
                    
                    Text(
                        text = statusText,
                        style = MaterialTheme.typography.headlineMedium,
                        color = statusColor,
                        fontWeight = FontWeight.Bold
                    )
                }
                
                if (countdownRemainingSec > 0) {
                    Spacer(modifier = Modifier.height(4.dp))
                    Text(
                        text = "Time Left: ${countdownRemainingSec / 60}m ${countdownRemainingSec % 60}s",
                        style = MaterialTheme.typography.bodyMedium,
                        color = CyanPrimary,
                        fontWeight = FontWeight.Bold
                    )
                }
            }

            // Glowing Halo Status Indicator
            Box(
                modifier = Modifier.size(64.dp),
                contentAlignment = Alignment.Center
            ) {
                if (isPumpRunning && !isStale) {
                    Box(
                        modifier = Modifier
                            .size(64.dp)
                            .scale(haloScale)
                            .clip(CircleShape)
                            .background(EmeraldSecondary.copy(alpha = 0.2f))
                    )
                }
                Box(
                    modifier = Modifier
                        .size(48.dp)
                        .clip(CircleShape)
                        .background(
                            if (isStale) GrayText.copy(alpha = 0.3f)
                            else if (isPumpRunning) EmeraldSecondary
                            else Color.DarkGray
                        )
                )
            }
        }

        // Flow rate telemetry strip
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .background(Color.Black.copy(alpha = 0.2f))
                .padding(horizontal = 24.dp, vertical = 12.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(
                    text = "Flow Rate",
                    style = MaterialTheme.typography.bodyMedium,
                    color = GrayText
                )
                Text(
                    text = if (isStale) "-- L/min" else String.format("%.1f L/min", flowRateLpm),
                    style = MaterialTheme.typography.labelLarge,
                    color = if (isStale) GrayText else MaterialTheme.colorScheme.onSurface
                )
            }
        }
    }
}
