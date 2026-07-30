package com.smartflow.service

import android.util.Log
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.messaging.FirebaseMessagingService
import com.google.firebase.messaging.RemoteMessage

class SmartFlowMessagingService : FirebaseMessagingService() {

    override fun onNewToken(token: String) {
        super.onNewToken(token)
        Log.d("FCM", "New token received: $token")
        
        // Save the token to RTDB under the user's notification_prefs/fcmTokens
        val currentUser = FirebaseAuth.getInstance().currentUser
        if (currentUser != null) {
            val uid = currentUser.uid
            val db = FirebaseDatabase.getInstance()
            val tokensRef = db.getReference("users/$uid/notification_prefs/fcmTokens")
            // Use hashCode as a simple safe key
            tokensRef.child(token.hashCode().toString()).setValue(token)
        }
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
