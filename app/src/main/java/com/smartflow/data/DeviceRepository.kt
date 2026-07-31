package com.smartflow.data

import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.flowOf
import com.smartflow.domain.DeviceEvent

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

    fun getUserDevicesStream(uid: String): Flow<List<String>> {
        return cloudStore.observeUserDevices(uid)
    }

    fun streamAllEvents(deviceIds: List<String>): Flow<List<Pair<String, DeviceEvent>>> {
        if (deviceIds.isEmpty()) return flowOf(emptyList())
        val flows = deviceIds.map { deviceId ->
            cloudStore.streamEvents(deviceId).map { events -> events.map { deviceId to it } }
        }
        return combine(flows) { listOfEventLists ->
            listOfEventLists.toList().flatten().sortedByDescending { it.second.timestamp }
        }
    }

    fun getNotificationPrefsStream(uid: String): Flow<NotificationPrefs> {
        return cloudStore.observeNotificationPrefs(uid)
    }

    suspend fun updateNotificationPrefs(uid: String, prefs: NotificationPrefs) {
        cloudStore.updateNotificationPrefs(uid, prefs)
    }

    suspend fun markNotificationsAsRead(uid: String) {
        cloudStore.markNotificationsAsRead(uid)
    }

    suspend fun markSpecificNotificationsAsRead(uid: String, eventIds: List<String>) {
        cloudStore.markSpecificNotificationsAsRead(uid, eventIds)
    }

    suspend fun deleteSpecificNotifications(uid: String, eventIds: List<String>) {
        cloudStore.deleteSpecificNotifications(uid, eventIds)
    }
}
