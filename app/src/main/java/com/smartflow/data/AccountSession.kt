package com.smartflow.data

import com.google.firebase.auth.FirebaseUser

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
}
