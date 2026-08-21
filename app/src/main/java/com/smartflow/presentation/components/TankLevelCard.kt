package com.smartflow.presentation.components

import androidx.compose.animation.core.*
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.*
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.clipPath
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.unit.dp
import com.smartflow.domain.TelemetryValue
import com.smartflow.presentation.components.core.TelemetryMetric
import com.smartflow.ui.theme.LocalMotion
import com.smartflow.ui.theme.LocalSpacing
import kotlin.math.sin

@Composable
fun TankLevelCard(
    waterLevel: TelemetryValue<Int>,
    modifier: Modifier = Modifier
) {
    val isStale = waterLevel is TelemetryValue.Stale
    val waveColor = if (isStale) MaterialTheme.colorScheme.onSurfaceVariant else MaterialTheme.colorScheme.primary
    val tankBackgroundColor = MaterialTheme.colorScheme.surfaceContainerHighest
    val spacing = LocalSpacing.current
    val motion = LocalMotion.current

    val infiniteTransition = rememberInfiniteTransition()
    val phaseOffset by infiniteTransition.animateFloat(
        initialValue = 0f,
        targetValue = 2f * Math.PI.toFloat(),
        animationSpec = infiniteRepeatable(
            animation = tween(2000, easing = LinearEasing),
            repeatMode = RepeatMode.Restart
        ),
        label = "wave_phase"
    )

    // Smoothly animate water level changes
    val levelValue = when (waterLevel) {
        is TelemetryValue.Available -> waterLevel.value
        is TelemetryValue.Stale -> waterLevel.lastKnownValue
        is TelemetryValue.Unavailable -> 0
    }
    
    val animatedLevel by animateFloatAsState(
        targetValue = levelValue.coerceIn(0, 100) / 100f,
        animationSpec = tween(motion.durationExtraSlow * 2, easing = FastOutSlowInEasing),
        label = "water_level"
    )

    Card(
        modifier = modifier
            .fillMaxWidth()
            .height(200.dp)
            .clearAndSetSemantics {
                contentDescription = when (waterLevel) {
                    is TelemetryValue.Stale -> "Tank level ${waterLevel.lastKnownValue} percent, but data is stale"
                    is TelemetryValue.Unavailable -> "Tank level unavailable. Check level sensor."
                    is TelemetryValue.Available -> "Tank level ${waterLevel.value} percent"
                }
            },
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxSize()
                .padding(spacing.medium),
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
                                cornerRadius = androidx.compose.ui.geometry.CornerRadius(spacing.medium.toPx())
                            )
                        )
                    }

                    // Background empty tank
                    drawPath(
                        path = containerPath,
                        color = tankBackgroundColor
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

            Spacer(modifier = Modifier.width(spacing.large))

            // Text info using TelemetryMetric
            Box(
                modifier = Modifier.weight(1f),
                contentAlignment = Alignment.Center
            ) {
                TelemetryMetric(
                    label = "Tank Level",
                    telemetry = waterLevel,
                    formatValue = { "$it%" },
                    isLarge = true
                )
            }
        }
    }
}
