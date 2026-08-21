package com.smartflow.ui.theme

import androidx.compose.runtime.compositionLocalOf

data class Motion(
    val durationFast: Int = 100,
    val durationMedium: Int = 200,
    val durationSlow: Int = 300,
    val durationExtraSlow: Int = 500
)

val LocalMotion = compositionLocalOf { Motion() }
