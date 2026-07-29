package com.smartflow.presentation.components

import androidx.compose.animation.core.*
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.*
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.clipPath
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.smartflow.domain.ConnectionState
import com.smartflow.presentation.theme.CyanPrimary
import com.smartflow.presentation.theme.GrayText
import kotlin.math.sin

@Composable
fun TankLevelCard(
    waterLevelPct: Int,
    connectionState: ConnectionState,
    modifier: Modifier = Modifier
) {
    val isStale = connectionState != ConnectionState.CONNECTED
    val waveColor = if (isStale) GrayText else CyanPrimary

    val infiniteTransition = rememberInfiniteTransition()
    val phaseOffset by infiniteTransition.animateFloat(
        initialValue = 0f,
        targetValue = 2f * Math.PI.toFloat(),
        animationSpec = infiniteRepeatable(
            animation = tween(2000, easing = LinearEasing),
            repeatMode = RepeatMode.Restart
        )
    )

    // Smoothly animate water level changes
    val animatedLevel by animateFloatAsState(
        targetValue = waterLevelPct.coerceIn(0, 100) / 100f,
        animationSpec = tween(1000, easing = FastOutSlowInEasing)
    )

    Card(
        modifier = modifier
            .fillMaxWidth()
            .height(200.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxSize()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Tank visual
            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight()
            ) {
                Canvas(modifier = Modifier.fillMaxSize()) {
                    val width = size.width
                    val height = size.height

                    val containerPath = Path().apply {
                        addRoundRect(
                            androidx.compose.ui.geometry.RoundRect(
                                rect = Rect(0f, 0f, width, height),
                                cornerRadius = androidx.compose.ui.geometry.CornerRadius(16.dp.toPx())
                            )
                        )
                    }

                    // Background empty tank
                    drawPath(
                        path = containerPath,
                        color = Color.Black.copy(alpha = 0.2f)
                    )

                    clipPath(containerPath) {
                        val wavePath = Path().apply {
                            val waterY = height - (height * animatedLevel)
                            val amplitude = 10f
                            val frequency = 0.05f

                            moveTo(0f, height)
                            lineTo(0f, waterY)

                            // Only animate wave if it's connected and not empty/full
                            if (!isStale && animatedLevel > 0f && animatedLevel < 1f) {
                                for (x in 0..width.toInt() step 5) {
                                    val y = waterY + sin((x * frequency) + phaseOffset) * amplitude
                                    lineTo(x.toFloat(), y)
                                }
                            } else {
                                lineTo(width, waterY)
                            }

                            lineTo(width, height)
                            close()
                        }

                        drawPath(
                            path = wavePath,
                            color = waveColor.copy(alpha = 0.8f)
                        )
                    }
                }
            }

            Spacer(modifier = Modifier.width(24.dp))

            // Text info
            Column(
                modifier = Modifier.weight(1f),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center
            ) {
                Text(
                    text = "Tank Level",
                    style = MaterialTheme.typography.bodyMedium,
                    color = GrayText
                )
                Text(
                    text = "$waterLevelPct%",
                    style = MaterialTheme.typography.headlineLarge,
                    color = if (isStale) GrayText else MaterialTheme.colorScheme.onSurface,
                    fontWeight = FontWeight.Bold
                )
                if (isStale) {
                    Text(
                        text = "Stale Data",
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.error
                    )
                }
            }
        }
    }
}
