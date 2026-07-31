package com.smartflow.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.platform.LocalView
import androidx.core.view.WindowCompat
import android.app.Activity
import androidx.compose.runtime.SideEffect

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

val SmartFlowMonochromeDarkColorScheme = darkColorScheme(
    primary = Color(0xFFFFFFFF),
    onPrimary = Color(0xFF000000),
    primaryContainer = Color(0xFF333333),
    onPrimaryContainer = Color(0xFFFFFFFF),
    secondary = Color(0xFFCCCCCC),
    onSecondary = Color(0xFF000000),
    secondaryContainer = Color(0xFF444444),
    onSecondaryContainer = Color(0xFFFFFFFF),
    tertiary = Color(0xFF888888),
    onTertiary = Color(0xFF000000),
    tertiaryContainer = Color(0xFF555555),
    onTertiaryContainer = Color(0xFFFFFFFF),
    error = Color(0xFFFFFFFF),
    onError = Color(0xFF000000),
    errorContainer = Color(0xFF666666),
    onErrorContainer = Color(0xFFFFFFFF),
    background = BackgroundMono,
    onBackground = OnBackgroundMono,
    surface = SurfaceMono,
    onSurface = OnBackgroundMono,
    surfaceVariant = Color(0xFF222222),
    onSurfaceVariant = OnSurfaceVariantMono,
    surfaceContainerLow = SurfaceContainerLowMono,
    surfaceContainer = SurfaceContainerMono,
    surfaceContainerHigh = SurfaceContainerHighMono,
    surfaceContainerHighest = SurfaceContainerHighestMono,
    outline = OutlineMono,
    outlineVariant = Color(0xFF444444),
    scrim = Color(0xFF000000)
)

val SmartFlowMonochromeLightColorScheme = lightColorScheme(
    primary = Color(0xFF000000),
    onPrimary = Color(0xFFFFFFFF),
    primaryContainer = Color(0xFFCCCCCC),
    onPrimaryContainer = Color(0xFF000000),
    secondary = Color(0xFF333333),
    onSecondary = Color(0xFFFFFFFF),
    secondaryContainer = Color(0xFFBBBBBB),
    onSecondaryContainer = Color(0xFF000000),
    tertiary = Color(0xFF666666),
    onTertiary = Color(0xFFFFFFFF),
    tertiaryContainer = Color(0xFFAAAAAA),
    onTertiaryContainer = Color(0xFF000000),
    error = Color(0xFF000000),
    onError = Color(0xFFFFFFFF),
    errorContainer = Color(0xFF999999),
    onErrorContainer = Color(0xFF000000),
    background = Color(0xFFFFFFFF),
    onBackground = Color(0xFF000000),
    surface = Color(0xFFFFFFFF),
    onSurface = Color(0xFF000000),
    surfaceVariant = Color(0xFFEEEEEE),
    onSurfaceVariant = Color(0xFF555555),
    surfaceContainerLow = Color(0xFFEEEEEE),
    surfaceContainer = Color(0xFFDDDDDD),
    surfaceContainerHigh = Color(0xFFCCCCCC),
    surfaceContainerHighest = Color(0xFFBBBBBB),
    outline = Color(0xFF999999),
    outlineVariant = Color(0xFFCCCCCC),
    scrim = Color(0xFF000000)
)

enum class ThemePreference {
    SYSTEM_DEFAULT,
    LIGHT,
    DARK,
    MONOCHROME_LIGHT,
    MONOCHROME_DARK
}

val LocalThemePreference = androidx.compose.runtime.staticCompositionLocalOf { ThemePreference.SYSTEM_DEFAULT }

@Composable
fun SmartFlowTheme(
    themePreference: ThemePreference = ThemePreference.SYSTEM_DEFAULT,
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit,
) {
    val colorScheme = when (themePreference) {
        ThemePreference.LIGHT -> SmartFlowLightColorScheme
        ThemePreference.DARK -> SmartFlowDarkColorScheme
        ThemePreference.MONOCHROME_LIGHT -> SmartFlowMonochromeLightColorScheme
        ThemePreference.MONOCHROME_DARK -> SmartFlowMonochromeDarkColorScheme
        ThemePreference.SYSTEM_DEFAULT -> if (darkTheme) SmartFlowDarkColorScheme else SmartFlowLightColorScheme
    }
    val view = LocalView.current
    if (!view.isInEditMode) {
        SideEffect {
            val window = (view.context as Activity).window
            window.statusBarColor = colorScheme.background.toArgb()
            
            val isLightMode = colorScheme == SmartFlowLightColorScheme || colorScheme == SmartFlowMonochromeLightColorScheme
            WindowCompat.getInsetsController(window, view).isAppearanceLightStatusBars = isLightMode
        }
    }

    androidx.compose.runtime.CompositionLocalProvider(
        LocalThemePreference provides themePreference
    ) {
        MaterialTheme(
            colorScheme = colorScheme,
            typography = SmartFlowTypography,
            shapes = SmartFlowShapes,
            content = content,
        )
    }
}
