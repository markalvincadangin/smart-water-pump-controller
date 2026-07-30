package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.data.DeviceEvent
import com.smartflow.data.DeviceRepository
import com.smartflow.data.NotificationPrefs
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.flatMapLatest
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.flow.flowOf
import kotlinx.coroutines.launch

class NotificationsViewModel(
    private val uid: String,
    private val deviceRepository: DeviceRepository
) : ViewModel() {

    @kotlinx.coroutines.ExperimentalCoroutinesApi
    val events: StateFlow<List<Pair<String, DeviceEvent>>> = deviceRepository.getUserDevicesStream(uid)
        .flatMapLatest { deviceIds ->
            if (deviceIds.isEmpty()) flowOf(emptyList())
            else deviceRepository.streamAllEvents(deviceIds)
        }
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), emptyList())

    val prefs: StateFlow<NotificationPrefs> = deviceRepository.getNotificationPrefsStream(uid)
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), NotificationPrefs())

    fun markAllAsRead() {
        viewModelScope.launch {
            deviceRepository.markNotificationsAsRead(uid)
        }
    }
}
