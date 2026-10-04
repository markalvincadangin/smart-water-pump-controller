package com.smartflow.service

import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.os.Build
import com.smartflow.R

object NotificationChannels {
    const val PUMP_ALERTS_ID = "pump_alerts"

    fun create(context: Context) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return

        val channel = NotificationChannel(
            PUMP_ALERTS_ID,
            context.getString(R.string.notification_channel_pump_alerts_name),
            NotificationManager.IMPORTANCE_HIGH
        ).apply {
            description = context.getString(R.string.notification_channel_pump_alerts_description)
            enableVibration(true)
            setShowBadge(true)
        }

        context.getSystemService(NotificationManager::class.java)
            .createNotificationChannel(channel)
    }
}
