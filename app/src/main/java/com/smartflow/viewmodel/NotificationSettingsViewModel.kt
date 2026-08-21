package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.data.DeviceRepository
import com.smartflow.data.NotificationPrefs
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class NotificationSettingsViewModel(
    private val uid: String,
    private val deviceRepository: DeviceRepository
) : ViewModel() {

    val prefs: StateFlow<NotificationPrefs> = deviceRepository.getNotificationPrefsStream(uid)
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), NotificationPrefs())

    fun updatePrefs(newPrefs: NotificationPrefs) {
        viewModelScope.launch {
            deviceRepository.updateNotificationPrefs(uid, newPrefs)
        }
    }
}
