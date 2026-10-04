package com.smartflow.service

import android.app.PendingIntent
import android.content.Intent
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import com.google.firebase.messaging.FirebaseMessagingService
import com.google.firebase.messaging.RemoteMessage
import com.smartflow.MainActivity
import com.smartflow.R

class SmartFlowMessagingService : FirebaseMessagingService() {

    override fun onNewToken(token: String) {
        super.onNewToken(token)
        Log.d("FCM", "New token received")

        FcmTokenRegistrar.registerToken(token)
    }

    override fun onMessageReceived(message: RemoteMessage) {
        super.onMessageReceived(message)

        val payload = FcmNotificationPayload.from(message)
        if (payload == null) {
            Log.w("FCM", "Ignoring malformed foreground notification payload")
            return
        }

        Log.d(
            "FCM",
            "Foreground notification received: tag=${payload.tag}, " +
                "eventCode=${payload.eventCode}, eventId=${payload.eventId}, deviceId=${payload.deviceId}"
        )

        NotificationChannels.create(this)

        val launchIntent = Intent(this, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_SINGLE_TOP
        }
        val pendingIntent = PendingIntent.getActivity(
            this,
            0,
            launchIntent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val notificationId = stableNotificationId(payload)

        val notification = NotificationCompat.Builder(this, NotificationChannels.PUMP_ALERTS_ID)
            .setSmallIcon(R.drawable.logo_stacked_white)
            .setContentTitle(payload.title)
            .setContentText(payload.body)
            .setStyle(NotificationCompat.BigTextStyle().bigText(payload.body))
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setAutoCancel(true)
            .setContentIntent(pendingIntent)
            .setCategory(NotificationCompat.CATEGORY_ALARM)
            .build()

        NotificationManagerCompat.from(this).notify(notificationId, notification)
    }

    private fun stableNotificationId(payload: FcmNotificationPayload): Int {
        val key = listOf(
            payload.deviceId,
            payload.eventId,
            payload.eventCode,
            payload.tag,
            payload.title,
            payload.body
        ).joinToString("|")
        return key.hashCode()
    }
}
