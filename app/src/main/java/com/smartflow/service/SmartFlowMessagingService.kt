package com.smartflow.service

import android.util.Log
import com.google.firebase.messaging.FirebaseMessagingService
import com.google.firebase.messaging.RemoteMessage

class SmartFlowMessagingService : FirebaseMessagingService() {

    override fun onNewToken(token: String) {
        super.onNewToken(token)
        Log.d("FCM", "New token received")

        FcmTokenRegistrar.registerToken(token)
    }

    override fun onMessageReceived(message: RemoteMessage) {
        super.onMessageReceived(message)
        Log.d("FCM", "Message received from: ${message.from}")
        if (message.notification != null) {
            Log.d("FCM", "Notification Title: ${message.notification?.title}")
            Log.d("FCM", "Notification Body: ${message.notification?.body}")
        }
    }
}
