package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.data.DeviceRepository
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.flatMapLatest
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.flow.flowOf

class DeviceListViewModel(
    uid: String,
    deviceRepository: DeviceRepository
) : ViewModel() {

    val devices: StateFlow<List<String>?> = deviceRepository.getUserDevicesStream(uid)
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), null)

    @kotlinx.coroutines.ExperimentalCoroutinesApi
    val hasUnreadNotifications: StateFlow<Boolean> = devices
        .flatMapLatest { deviceIds ->
            if (deviceIds == null || deviceIds.isEmpty()) return@flatMapLatest flowOf(false)
            combine(
                deviceRepository.streamAllEvents(deviceIds),
                deviceRepository.getNotificationPrefsStream(uid)
            ) { events, prefs ->
                events.any { (deviceId, event) ->
                    val isDeleted = prefs.deletedEventIds[event.id] == true
                    val isRead = prefs.readEventIds[event.id] == true || event.timestamp <= prefs.lastReadTimestamp
                    !isDeleted && !isRead
                }
            }
        }
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), false)
}
