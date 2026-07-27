package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.data.BleProvisioningClient
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import io.reactivex.rxjava3.disposables.Disposable
import io.reactivex.rxjava3.android.schedulers.AndroidSchedulers
import io.reactivex.rxjava3.schedulers.Schedulers

class ProvisioningViewModel(
    private val bleClient: BleProvisioningClient
) : ViewModel() {

    private val _provisioningState = MutableStateFlow<ProvisioningState>(ProvisioningState.Idle)
    val provisioningState: StateFlow<ProvisioningState> = _provisioningState

    private var scanDisposable: Disposable? = null
    private var provisionDisposable: Disposable? = null

    fun startScanning() {
        _provisioningState.value = ProvisioningState.Scanning
        scanDisposable?.dispose()
        scanDisposable = bleClient.scanForDevices()
            .subscribeOn(Schedulers.io())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe({ macAddress ->
                _provisioningState.value = ProvisioningState.DeviceFound(macAddress)
                scanDisposable?.dispose() // Stop scanning once we found a SmartFlow device
            }, { error ->
                _provisioningState.value = ProvisioningState.Error("Scan failed: ${error.message}")
            })
    }

    fun provisionDevice(macAddress: String, ssid: String, pass: String) {
        _provisioningState.value = ProvisioningState.Provisioning
        provisionDisposable?.dispose()
        provisionDisposable = bleClient.provisionDevice(macAddress, ssid, pass)
            .subscribeOn(Schedulers.io())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe({ token ->
                _provisioningState.value = ProvisioningState.Success(token)
            }, { error ->
                _provisioningState.value = ProvisioningState.Error("Provisioning failed: ${error.message}")
            })
    }

    override fun onCleared() {
        super.onCleared()
        scanDisposable?.dispose()
        provisionDisposable?.dispose()
    }
}

sealed class ProvisioningState {
    object Idle : ProvisioningState()
    object Scanning : ProvisioningState()
    data class DeviceFound(val macAddress: String) : ProvisioningState()
    object Provisioning : ProvisioningState()
    data class Success(val claimToken: String) : ProvisioningState()
    data class Error(val message: String) : ProvisioningState()
}
