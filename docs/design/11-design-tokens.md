# Design Tokens

This document serves as the bridge between the design specifications and the Jetpack Compose implementation. These are the raw tokens to be implemented in `app/src/main/java/com/smartflow/ui/theme/`.

## 1. Color Tokens (`Color.kt`)

```kotlin
package com.smartflow.ui.theme
import androidx.compose.ui.graphics.Color

// Brand Core
val BluePrimary = Color(0xFF0EA5E9)
val GreenSecondary = Color(0xFF10B981)
val NavyBackground = Color(0xFF0F172A)
val WhiteAccent = Color(0xFFFFFFFF)

// Semantic Core
val RedError = Color(0xFFEF4444)
val AmberWarning = Color(0xFFF59E0B)

// Dark Theme Surfaces
val SurfaceDark = Color(0xFF1E293B)
val SurfaceVariantDark = Color(0xFF334155)
val OutlineDark = Color(0xFF475569)

// Light Theme Surfaces
val SurfaceLight = Color(0xFFFFFFFF)
val SurfaceVariantLight = Color(0xFFF1F5F9)
val OutlineLight = Color(0xFFCBD5E1)
val BackgroundLight = Color(0xFFF8FAFC)
```

## 2. Typography Tokens (`Type.kt`)

```kotlin
package com.smartflow.ui.theme
import androidx.compose.material3.Typography
import androidx.compose.ui.text.font.Font
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import com.smartflow.R

val Inter = FontFamily(
    Font(R.font.inter_regular, FontWeight.Normal),
    Font(R.font.inter_medium, FontWeight.Medium),
    Font(R.font.inter_semibold, FontWeight.SemiBold),
    Font(R.font.inter_bold, FontWeight.Bold)
)

val Roboto = FontFamily(
    Font(R.font.roboto_regular, FontWeight.Normal)
)

// The Typography object scales these per the M3 specification
val SmartFlowTypography = Typography(
    // Example: Display Large
    displayLarge = TextStyle(
        fontFamily = Inter,
        fontWeight = FontWeight.Bold,
        fontSize = 57.sp,
        lineHeight = 64.sp,
        letterSpacing = (-0.25).sp
    ),
    // Example: Body Large
    bodyLarge = TextStyle(
        fontFamily = Roboto,
        fontWeight = FontWeight.Normal,
        fontSize = 16.sp,
        lineHeight = 24.sp,
        letterSpacing = 0.5.sp
    )
    // ... Implement remaining scales based on 04-typography.md
)
```

## 3. Theme Implementation (`Theme.kt`)

```kotlin
private val DarkColorScheme = darkColorScheme(
    primary = BluePrimary,
    onPrimary = WhiteAccent,
    secondary = GreenSecondary,
    onSecondary = WhiteAccent,
    background = NavyBackground,
    surface = SurfaceDark,
    error = RedError,
    // ... complete mapping
)

private val LightColorScheme = lightColorScheme(
    primary = BluePrimary,
    onPrimary = WhiteAccent,
    secondary = GreenSecondary,
    onSecondary = WhiteAccent,
    background = BackgroundLight,
    surface = SurfaceLight,
    error = RedError,
    // ... complete mapping
)
```
