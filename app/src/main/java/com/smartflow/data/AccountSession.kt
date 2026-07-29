package com.smartflow.data

import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.FirebaseUser
import kotlinx.coroutines.tasks.await

object AccountSession {
    fun state(user: FirebaseUser?): DurableAccountState {
        if (user == null) return DurableAccountState.SIGNED_OUT
        if (user.isAnonymous) return DurableAccountState.ANONYMOUS
        val provider = user.providerData.firstOrNull { it.providerId != "firebase" }?.providerId
        return if (provider == "password" && !user.isEmailVerified) {
            DurableAccountState.UNVERIFIED_EMAIL
        } else {
            DurableAccountState.ELIGIBLE
        }
    }

    /**
     * Confirms that the locally cached Firebase user still exists before an
     * ownership-sensitive operation. A project reset can delete a user while
     * the Android SDK still restores its local session on the next launch.
     */
    suspend fun refreshDurableState(auth: FirebaseAuth): DurableAccountState {
        val user = auth.currentUser ?: return DurableAccountState.SIGNED_OUT
        return try {
            user.getIdToken(true).await()
            user.reload().await()
            state(auth.currentUser)
        } catch (_: Exception) {
            auth.signOut()
            DurableAccountState.SIGNED_OUT
        }
    }
}
