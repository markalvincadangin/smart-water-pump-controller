package com.smartflow.service

import android.content.Intent

data class NotificationRouteTarget(
    val deviceId: String?,
    val eventId: String?,
    val eventCode: String?,
    val tag: String?
)

const val EXTRA_SMARTFLOW_NOTIFICATION = "smartflow_notification"
const val EXTRA_NOTIFICATION_DEVICE_ID = "deviceId"
const val EXTRA_NOTIFICATION_EVENT_ID = "eventId"
const val EXTRA_NOTIFICATION_EVENT_CODE = "eventCode"
const val EXTRA_NOTIFICATION_TAG = "tag"

fun notificationRouteTarget(
    isSmartflowNotification: Boolean,
    deviceId: String? = null,
    eventId: String? = null,
    eventCode: String? = null,
    tag: String? = null
): NotificationRouteTarget? {
    if (!isSmartflowNotification) return null

    val cleanDeviceId = deviceId?.takeIf { it.isNotBlank() }
    val cleanEventId = eventId?.takeIf { it.isNotBlank() }
    val cleanEventCode = eventCode?.takeIf { it.isNotBlank() }
    val cleanTag = tag?.takeIf { it.isNotBlank() }

    if (cleanDeviceId == null && cleanEventId == null && cleanEventCode == null && cleanTag == null) return null
    return NotificationRouteTarget(cleanDeviceId, cleanEventId, cleanEventCode, cleanTag)
}

fun notificationRouteTarget(intent: Intent?): NotificationRouteTarget? {
    if (intent == null) return null
    return notificationRouteTarget(
        isSmartflowNotification = intent.getBooleanExtra(EXTRA_SMARTFLOW_NOTIFICATION, false),
        deviceId = intent.getStringExtra(EXTRA_NOTIFICATION_DEVICE_ID),
        eventId = intent.getStringExtra(EXTRA_NOTIFICATION_EVENT_ID),
        eventCode = intent.getStringExtra(EXTRA_NOTIFICATION_EVENT_CODE),
        tag = intent.getStringExtra(EXTRA_NOTIFICATION_TAG)
    )
}

