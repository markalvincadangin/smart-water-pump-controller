# Data Model: App Design System

**Feature**: 009-app-design-system

There are no database entities or domain data models introduced in this feature. This feature strictly implements Jetpack Compose UI theming (Colors, Typography, Shapes).

## State Models
The `SmartFlowTheme` depends on the system's `isSystemInDarkTheme()` boolean to determine which color scheme to apply. No persistent state is added.
