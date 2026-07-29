package com.smartflow.presentation.theme

import android.app.Activity
import android.os.Build
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.dynamicDarkColorScheme
import androidx.compose.material3.dynamicLightColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalView
import androidx.core.view.WindowCompat

// The SmartFlow app is strictly a Dark Theme app per the Google Stitch design
// For M3 completeness, we define the dark color scheme using the specific tokens.
private val SmartFlowDarkColorScheme = darkColorScheme(
    primary = CyanPrimary,
    secondary = EmeraldSecondary,
    tertiary = AmberWarning,
    error = RedError,
    background = DarkSlateSurface,
    surface = DarkSlateSurface,
    surfaceVariant = DarkSlateSurfaceVariant,
    onPrimary = WhiteText,
    onSecondary = WhiteText,
    onTertiary = DarkSlateSurface,
    onError = WhiteText,
    onBackground = WhiteText,
    onSurface = WhiteText,
    onSurfaceVariant = WhiteText
)

@Composable
fun SmartFlowTheme(
    // We enforce Dark Theme by default for the HMI dashboard.
    darkTheme: Boolean = true,
    // Dynamic color is available on Android 12+, but we disable it to keep strict safety colors.
    dynamicColor: Boolean = false,
    content: @Composable () -> Unit
) {
    val colorScheme = SmartFlowDarkColorScheme
    
    val view = LocalView.current
    if (!view.isInEditMode) {
        SideEffect {
            val window = (view.context as Activity).window
            window.statusBarColor = colorScheme.background.toArgb()
            WindowCompat.getInsetsController(window, view).isAppearanceLightStatusBars = !darkTheme
        }
    }

    MaterialTheme(
        colorScheme = colorScheme,
        typography = AppTypography,
        content = content
    )
}
