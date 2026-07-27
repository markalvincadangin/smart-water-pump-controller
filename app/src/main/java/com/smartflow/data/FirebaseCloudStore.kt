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

class FirebaseCloudStore {
    private val database = FirebaseDatabase.getInstance().reference

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

    suspend fun claimDevice(uid: String, deviceId: String) {
        val updates = mapOf(
            "users/$uid/devices/$deviceId" to true,
            "devices/$deviceId/metadata/claimedByUid" to uid
        )
        database.updateChildren(updates).await()
    }

    suspend fun unclaimDevice(uid: String, deviceId: String) {
        database.child("users").child(uid).child("devices").child(deviceId).removeValue().await()
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
