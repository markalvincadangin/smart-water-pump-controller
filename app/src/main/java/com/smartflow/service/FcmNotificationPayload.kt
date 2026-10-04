package com.smartflow.service

import com.google.firebase.messaging.RemoteMessage

/**
 * Normalized foreground FCM payload.
 *
 * Cloud Functions owns delivery policy. This parser only normalizes the
 * already-delivered message for Android presentation.
 */
data class FcmNotificationPayload(
    val title: String,
    val body: String,
    val tag: String?,
    val eventCode: String?,
    val eventId: String?,
    val deviceId: String?
) {
    companion object {
        fun from(
            notificationTitle: String? = null,
            notificationBody: String? = null,
            data: Map<String, String> = emptyMap()
        ): FcmNotificationPayload? {
            val title = notificationTitle?.takeIf { it.isNotBlank() }
                ?: data["title"]?.takeIf { it.isNotBlank() }
                ?: return null
            val body = notificationBody?.takeIf { it.isNotBlank() }
                ?: data["body"]?.takeIf { it.isNotBlank() }
                ?: return null

            return FcmNotificationPayload(
                title = title,
                body = body,
                tag = data["tag"]?.takeIf { it.isNotBlank() },
                eventCode = data["eventCode"]?.takeIf { it.isNotBlank() },
                eventId = data["eventId"]?.takeIf { it.isNotBlank() },
                deviceId = data["deviceId"]?.takeIf { it.isNotBlank() }
            )
        }

        fun from(message: RemoteMessage): FcmNotificationPayload? {
            return from(
                notificationTitle = message.notification?.title,
                notificationBody = message.notification?.body,
                data = message.data ?: emptyMap()
            )
        }
    }
}
