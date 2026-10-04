package com.smartflow.service

import android.content.Intent

data class NotificationRouteTarget(
    val deviceId: String?,
    val eventId: String?,
    val eventCode: String?,
    val tag: String?
)

private const val EXTRA_NOTIFICATION_DEVICE_ID = "deviceId"
private const val EXTRA_NOTIFICATION_EVENT_ID = "eventId"
private const val EXTRA_NOTIFICATION_EVENT_CODE = "eventCode"
private const val EXTRA_NOTIFICATION_TAG = "tag"

fun notificationRouteTarget(intent: Intent?): NotificationRouteTarget? {
    if (intent?.getBooleanExtra("smartflow_notification", false) != true) return null

    val deviceId = intent.getStringExtra(EXTRA_NOTIFICATION_DEVICE_ID)?.takeIf { it.isNotBlank() }
    val eventId = intent.getStringExtra(EXTRA_NOTIFICATION_EVENT_ID)?.takeIf { it.isNotBlank() }
    val eventCode = intent.getStringExtra(EXTRA_NOTIFICATION_EVENT_CODE)?.takeIf { it.isNotBlank() }
    val tag = intent.getStringExtra(EXTRA_NOTIFICATION_TAG)?.takeIf { it.isNotBlank() }

    if (deviceId == null && eventId == null && eventCode == null && tag == null) return null
    return NotificationRouteTarget(deviceId, eventId, eventCode, tag)
}
