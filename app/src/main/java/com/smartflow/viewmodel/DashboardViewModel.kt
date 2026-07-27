package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.data.Device
import com.smartflow.data.DeviceRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

class DashboardViewModel(
    private val deviceRepository: DeviceRepository,
    private val deviceId: String
) : ViewModel() {

    private val _deviceState = MutableStateFlow<Device?>(null)
    val deviceState: StateFlow<Device?> = _deviceState

    init {
        viewModelScope.launch {
            deviceRepository.getDeviceStream(deviceId).collectLatest { device ->
                _deviceState.value = device
            }
        }
    }

    fun togglePump(currentState: Boolean) {
        viewModelScope.launch {
            deviceRepository.setPumpState(deviceId, !currentState)
        }
    }
}
