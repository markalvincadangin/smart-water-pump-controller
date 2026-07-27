package com.smartflow.data

import kotlinx.coroutines.flow.Flow

class DeviceRepository(private val cloudStore: FirebaseCloudStore) {
    
    fun getDeviceStream(deviceId: String): Flow<Device?> {
        return cloudStore.observeDevice(deviceId)
    }

    suspend fun setPumpState(deviceId: String, isOn: Boolean, mode: String = "MANUAL") {
        cloudStore.updateDesiredShadow(deviceId, ShadowState(pumpState = isOn, mode = mode))
    }

    fun getEventsStream(deviceId: String): Flow<List<DeviceEvent>> {
        return cloudStore.streamEvents(deviceId)
    }

    suspend fun unclaimDevice(uid: String, deviceId: String) {
        cloudStore.unclaimDevice(uid, deviceId)
    }
}
