package com.smartflow.service

import android.util.Log
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.FirebaseDatabase

/**
 * Registers the current app installation's FCM token for the signed-in user.
 *
 * The user-level path is the only supported token authority:
 * users/{uid}/notification_prefs/fcmTokens/{tokenId}
 */
object FcmTokenRegistrar {

    fun registerToken(token: String) {
        val user = FirebaseAuth.getInstance().currentUser
        if (user == null) {
            Log.d("FCM", "Skipping token registration because no user is signed in")
            return
        }

        val tokenId = token.hashCode().toString()
        val db = FirebaseDatabase.getInstance()
        val tokensRef = db.getReference(
            "users/${user.uid}/notification_prefs/fcmTokens"
        )

        tokensRef.child(tokenId).setValue(token)
            .addOnFailureListener { error ->
                Log.w("FCM", "Failed to register FCM token", error)
            }

        db.getReference("users/${user.uid}/notification_prefs/enabled")
            .setValue(true)
            .addOnFailureListener { error ->
                Log.w("FCM", "Failed to enable FCM notifications", error)
            }
    }
}
