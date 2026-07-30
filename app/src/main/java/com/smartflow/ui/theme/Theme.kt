package com.smartflow.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.foundation.isSystemInDarkTheme

private val SmartFlowDarkColorScheme = darkColorScheme(
    primary = BluePrimary,
    onPrimary = OnBrightBrand,
    secondary = GreenSecondary,
    onSecondary = OnBrightBrand,
    tertiary = WarningAmber,
    onTertiary = OnWarning,
    error = CriticalAction,
    onError = WhiteAccent,
    background = BackgroundDark,
    onBackground = OnBackgroundDark,
    surface = SurfaceDark,
    onSurface = OnBackgroundDark,
    surfaceContainerLow = SurfaceContainerLowDark,
    surfaceContainer = SurfaceContainerDark,
    surfaceContainerHigh = SurfaceContainerHighDark,
    surfaceContainerHighest = SurfaceContainerHighestDark,
    onSurfaceVariant = OnSurfaceVariantDark,
    outline = OutlineDark,
)

private val SmartFlowLightColorScheme = lightColorScheme(
    primary = BluePrimary,
    onPrimary = OnBrightBrand,
    secondary = GreenSecondary,
    onSecondary = OnBrightBrand,
    tertiary = WarningAmber,
    onTertiary = OnWarning,
    error = CriticalAction,
    onError = WhiteAccent,
    background = BackgroundLight,
    onBackground = OnBackgroundLight,
    surface = SurfaceLight,
    onSurface = OnBackgroundLight,
    surfaceContainerLow = SurfaceContainerLowLight,
    surfaceContainer = SurfaceContainerLight,
    surfaceContainerHigh = SurfaceContainerHighLight,
    surfaceContainerHighest = SurfaceContainerHighestLight,
    onSurfaceVariant = OnSurfaceVariantLight,
    outline = OutlineLight,
)

@Composable
fun SmartFlowTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit,
) {
    MaterialTheme(
        colorScheme = if (darkTheme) {
            SmartFlowDarkColorScheme
        } else {
            SmartFlowLightColorScheme
        },
        typography = SmartFlowTypography,
        shapes = SmartFlowShapes,
        content = content,
    )
}
