package com.smartflow.data

import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.tasks.await
import com.google.firebase.database.ChildEventListener
import com.google.firebase.functions.FirebaseFunctions
import com.google.firebase.functions.FirebaseFunctionsException

class FirebaseCloudStore {
    private val database = FirebaseDatabase.getInstance().reference
    private val functions = FirebaseFunctions.getInstance("asia-southeast1")

    fun observeDevice(deviceId: String): Flow<Device?> = callbackFlow {
        val ref = database.child("devices").child(deviceId)
        val listener = object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                try {
                    val device = snapshot.getValue(Device::class.java)
                    trySend(device?.copy(id = deviceId))
                } catch (e: Exception) {
                    trySend(null)
                }
            }

            override fun onCancelled(error: DatabaseError) {
                close(error.toException())
            }
        }
        ref.addValueEventListener(listener)
        awaitClose { ref.removeEventListener(listener) }
    }

    suspend fun updateDesiredShadow(deviceId: String, desired: ShadowState) {
        val updates = mapOf(
            "devices/$deviceId/shadow/desired/pumpState" to desired.pumpState,
            "devices/$deviceId/shadow/desired/mode" to desired.mode
        )
        database.updateChildren(updates).await()
    }

    suspend fun claimDevice(deviceId: String, pairingProof: String): OwnershipClaimResult {
        return try {
            @Suppress("UNCHECKED_CAST")
            val data = functions
                .getHttpsCallable("claimDevice")
                .call(mapOf("deviceId" to deviceId, "pairingProof" to pairingProof))
                .await()
                .data as? Map<String, Any?>
            OwnershipClaimResult.Claimed(
                deviceId = data?.get("deviceId") as? String ?: deviceId,
                auditId = data?.get("auditId") as? String
            )
        } catch (error: FirebaseFunctionsException) {
            val code = error.message?.substringAfterLast(": ")?.trim().orEmpty()
            when (error.code) {
                FirebaseFunctionsException.Code.UNAVAILABLE,
                FirebaseFunctionsException.Code.DEADLINE_EXCEEDED -> OwnershipClaimResult.Retryable(code.ifEmpty { error.code.name })
                FirebaseFunctionsException.Code.FAILED_PRECONDITION -> {
                    if (code == "CLAIM_UNAVAILABLE") OwnershipClaimResult.Retryable(code)
                    else OwnershipClaimResult.Rejected(code.ifEmpty { error.code.name })
                }
                else -> OwnershipClaimResult.Rejected(code.ifEmpty { error.code.name })
            }
        }
    }

    suspend fun requestWifiReprovision(deviceId: String) {
        functions.getHttpsCallable("requestWifiReprovision").call(mapOf("deviceId" to deviceId)).await()
    }

    suspend fun checkAccountDeletionEligibility(): Pair<Boolean, Int> {
        @Suppress("UNCHECKED_CAST")
        val data = functions.getHttpsCallable("checkAccountDeletionEligibility").call().await().data as? Map<String, Any?>
        return Pair(data?.get("eligible") as? Boolean ?: false, (data?.get("ownedDeviceCount") as? Number)?.toInt() ?: 0)
    }

    suspend fun startOwnershipTransfer(deviceId: String, recipientUid: String) {
        functions.getHttpsCallable("startOwnershipTransfer")
            .call(mapOf("deviceId" to deviceId, "recipientUid" to recipientUid)).await()
    }

    suspend fun releaseDevice(deviceId: String) {
        functions.getHttpsCallable("releaseDevice").call(mapOf("deviceId" to deviceId)).await()
    }

    suspend fun cancelOwnershipPairing(deviceId: String) {
        functions.getHttpsCallable("cancelOwnershipPairing").call(mapOf("deviceId" to deviceId)).await()
    }

    fun observeUserDevices(uid: String): Flow<List<String>> = callbackFlow {
        val ref = database.child("users").child(uid).child("devices")
        val listener = object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val devices = mutableListOf<String>()
                for (child in snapshot.children) {
                    if (child.getValue(Boolean::class.java) == true) {
                        child.key?.let { devices.add(it) }
                    }
                }
                trySend(devices.toList())
            }

            override fun onCancelled(error: DatabaseError) {
                close(error.toException())
            }
        }
        ref.addValueEventListener(listener)
        awaitClose { ref.removeEventListener(listener) }
    }

    fun streamEvents(deviceId: String): Flow<List<DeviceEvent>> = callbackFlow {
        val ref = database.child("devices").child(deviceId).child("events").orderByChild("timestamp").limitToLast(50)
        val eventsList = mutableListOf<DeviceEvent>()

        val listener = object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                eventsList.clear()
                for (child in snapshot.children) {
                    val ev = child.getValue(DeviceEvent::class.java)
                    if (ev != null) {
                        eventsList.add(ev.copy(id = child.key ?: ""))
                    }
                }
                trySend(eventsList.toList())
            }

            override fun onCancelled(error: DatabaseError) {
                close(error.toException())
            }
        }
        
        ref.addValueEventListener(listener)
        awaitClose { ref.removeEventListener(listener) }
    }
}
