package com.smartflow.data

data class NotificationPrefs(
    val enabled: Boolean = true,
    val dndEnabled: Boolean = false,
    val dndStartHour: Int = 22,
    val dndEndHour: Int = 6,
    val lastReadTimestamp: Long = 0L,
    val fcmTokens: Map<String, String> = emptyMap()
)
