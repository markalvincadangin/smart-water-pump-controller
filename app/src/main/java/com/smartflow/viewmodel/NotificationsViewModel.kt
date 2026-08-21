package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.domain.DeviceEvent
import com.smartflow.data.DeviceRepository
import com.smartflow.data.NotificationPrefs
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.flatMapLatest
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.flow.flowOf
import kotlinx.coroutines.launch

class NotificationsViewModel(
    private val uid: String,
    private val deviceRepository: DeviceRepository
) : ViewModel() {

    val prefs: StateFlow<NotificationPrefs> = deviceRepository.getNotificationPrefsStream(uid)
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), NotificationPrefs())

    @kotlinx.coroutines.ExperimentalCoroutinesApi
    val events: StateFlow<List<Pair<String, DeviceEvent>>> = deviceRepository.getUserDevicesStream(uid)
        .flatMapLatest { deviceIds ->
            if (deviceIds.isEmpty()) flowOf(emptyList())
            else combine(
                deviceRepository.streamAllEvents(deviceIds),
                prefs
            ) { allEvents, currentPrefs ->
                allEvents.filter { (_, event) ->
                    currentPrefs.deletedEventIds[event.id] != true
                }
            }
        }
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), emptyList())

    private val _selectedEventIds = MutableStateFlow<Set<String>>(emptySet())
    val selectedEventIds = _selectedEventIds.asStateFlow()

    private val _selectionMode = MutableStateFlow(false)
    val selectionMode = _selectionMode.asStateFlow()

    fun toggleSelection(eventId: String) {
        val current = _selectedEventIds.value
        if (current.contains(eventId)) {
            val newSet = current - eventId
            _selectedEventIds.value = newSet
            if (newSet.isEmpty()) {
                _selectionMode.value = false
            }
        } else {
            _selectedEventIds.value = current + eventId
            _selectionMode.value = true
        }
    }

    fun selectAll() {
        val allIds = events.value.map { it.second.id }.toSet()
        _selectedEventIds.value = allIds
        if (allIds.isNotEmpty()) _selectionMode.value = true
    }

    fun clearSelection() {
        _selectedEventIds.value = emptySet()
        _selectionMode.value = false
    }

    fun deleteSelected() {
        val toDelete = _selectedEventIds.value.toList()
        if (toDelete.isNotEmpty()) {
            viewModelScope.launch {
                deviceRepository.deleteSpecificNotifications(uid, toDelete)
            }
        }
        clearSelection()
    }

    fun markSelectedAsRead() {
        val toRead = _selectedEventIds.value.toList()
        if (toRead.isNotEmpty()) {
            viewModelScope.launch {
                deviceRepository.markSpecificNotificationsAsRead(uid, toRead)
            }
        }
        clearSelection()
    }

    fun markAllAsRead() {
        viewModelScope.launch {
            deviceRepository.markNotificationsAsRead(uid)
        }
    }
}
