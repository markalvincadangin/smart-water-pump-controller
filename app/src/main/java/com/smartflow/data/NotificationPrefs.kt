package com.smartflow.data

data class NotificationPrefs(
    val enabled: Boolean = true,
    val dndEnabled: Boolean = false,
    val dndStartHour: Int = 22,
    val dndEndHour: Int = 6,
    val lastReadTimestamp: Long = 0L,
    val fcmTokens: Map<String, String> = emptyMap(),
    val readEventIds: Map<String, Boolean> = emptyMap(),
    val deletedEventIds: Map<String, Boolean> = emptyMap(),
    val dryRunAlert: Boolean = true,
    val lowLevelAlert: Boolean = true,
    val pumpStartedAlert: Boolean = true,
    val overflowAlert: Boolean = true
)
