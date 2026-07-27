package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.data.Device
import com.smartflow.data.DeviceEvent
import com.smartflow.data.DeviceRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import com.google.firebase.auth.FirebaseAuth
import com.smartflow.data.BleProvisioningClient
import io.reactivex.rxjava3.schedulers.Schedulers

class DashboardViewModel(
    private val deviceRepository: DeviceRepository,
    private val deviceId: String,
    private val bleClient: BleProvisioningClient? = null,
    private val deviceMacAddress: String? = null
) : ViewModel() {

    private val _deviceState = MutableStateFlow<Device?>(null)
    val deviceState: StateFlow<Device?> = _deviceState

    private val _eventsState = MutableStateFlow<List<DeviceEvent>>(emptyList())
    val eventsState: StateFlow<List<DeviceEvent>> = _eventsState

    init {
        viewModelScope.launch {
            deviceRepository.getDeviceStream(deviceId).collectLatest { device ->
                _deviceState.value = device
            }
        }
        viewModelScope.launch {
            deviceRepository.getEventsStream(deviceId).collectLatest { events ->
                _eventsState.value = events.sortedByDescending { it.timestamp }
            }
        }
    }

    fun togglePump(currentState: Boolean) {
        viewModelScope.launch {
            deviceRepository.setPumpState(deviceId, !currentState)
        }
    }

    fun factoryReset(onSuccess: () -> Unit, onError: (String) -> Unit = {}) {
        val uid = FirebaseAuth.getInstance().currentUser?.uid
        if (uid == null) {
            onError("Not logged in")
            return
        }
        viewModelScope.launch {
            // 1. Send BLE RESET to erase NVS and reboot device (best-effort)
            val mac = deviceMacAddress
            if (bleClient != null && mac != null) {
                try {
                    bleClient.sendFactoryReset(mac)
                        .subscribeOn(Schedulers.io())
                        .blockingGet() // Wait for write to complete
                } catch (e: Exception) {
                    // BLE device may be out of range — log and continue with Firebase unclaim
                }
            }
            // 2. Remove Firebase ownership claim (triggers Cloud Function to wipe device node)
            try {
                deviceRepository.unclaimDevice(uid, deviceId)
                onSuccess()
            } catch (e: Exception) {
                onError("Failed to unclaim device: ${e.message}")
            }
        }
    }
}
