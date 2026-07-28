package com.smartflow.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartflow.data.BleProvisioningClient
import com.smartflow.data.WifiNetwork
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.delay
import io.reactivex.rxjava3.disposables.Disposable
import io.reactivex.rxjava3.android.schedulers.AndroidSchedulers
import io.reactivex.rxjava3.schedulers.Schedulers
import com.smartflow.data.FirebaseCloudStore
import com.smartflow.data.AccountSession
import com.smartflow.data.DurableAccountState
import com.smartflow.data.OwnershipClaimResult
import com.google.firebase.auth.FirebaseAuth

class ProvisioningViewModel(
    private val bleClient: BleProvisioningClient,
    private val cloudStore: FirebaseCloudStore
) : ViewModel() {

    private val _provisioningState = MutableStateFlow<ProvisioningState>(ProvisioningState.Idle)
    val provisioningState: StateFlow<ProvisioningState> = _provisioningState

    private var scanDisposable: Disposable? = null
    private var provisionDisposable: Disposable? = null
    private var wifiScanDisposable: Disposable? = null

    private val scannedNetworks = mutableMapOf<String, WifiNetwork>() // Deduplicate by SSID

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

    fun scanWifiNetworks(macAddress: String) {
        _provisioningState.value = ProvisioningState.ScanningWifi
        scannedNetworks.clear()

        wifiScanDisposable?.dispose()
        wifiScanDisposable = bleClient.scanWifiNetworks(macAddress)
            .subscribeOn(Schedulers.io())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe({ json ->
                val type = json.optString("type")
                if (type == "scan_start") {
                    _provisioningState.value = ProvisioningState.ScanningWifi
                } else if (type == "network") {
                    val ssid = json.optString("ssid")
                    val bssid = json.optString("bssid")
                    val rssi = json.optInt("rssi")
                    val auth = json.optString("auth")

                    val existing = scannedNetworks[ssid]
                    if (existing == null || existing.rssi < rssi) {
                        scannedNetworks[ssid] = WifiNetwork(ssid, bssid, rssi, auth)
                        val sortedList = scannedNetworks.values.sortedByDescending { it.rssi }
                        _provisioningState.value = ProvisioningState.WifiListReceived(macAddress, sortedList, isScanning = true)
                    }
                } else if (type == "scan_complete") {
                    val sortedList = scannedNetworks.values.sortedByDescending { it.rssi }
                    _provisioningState.value = ProvisioningState.WifiListReceived(macAddress, sortedList, isScanning = false)
                    wifiScanDisposable?.dispose()
                } else if (type == "error") {
                    _provisioningState.value = ProvisioningState.Error(json.optString("message", "Scan Error"))
                    wifiScanDisposable?.dispose()
                }
            }, { error ->
                _provisioningState.value = ProvisioningState.Error("Wi-Fi Scan failed: ${error.message}")
            })
    }

    fun provisionDevice(macAddress: String, ssid: String, pass: String) {
        _provisioningState.value = ProvisioningState.Provisioning
        provisionDisposable?.dispose()

        var claimToken = ""
        var deviceId = ""

        provisionDisposable = bleClient.provisionDevice(macAddress, ssid, pass)
            .subscribeOn(Schedulers.io())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe({ json ->
                val type = json.optString("type")
                if (type == "token") {
                    claimToken = json.optString("value")
                    deviceId = json.optString("device_id", claimToken)
                } else if (type == "status") {
                    val status = json.optString("status")
                    if (status == "provisioned") {
                        // The firmware deliberately closes BLE shortly after this
                        // terminal status. Stop observing first so the expected
                        // GATT disconnect cannot replace a successful claim with
                        // a provisioning error.
                        provisionDisposable?.dispose()
                        val eligible = AccountSession.state(FirebaseAuth.getInstance().currentUser) == DurableAccountState.ELIGIBLE
                        if (eligible && deviceId.isNotEmpty() && claimToken.isNotEmpty()) {
                            viewModelScope.launch {
                                try {
                                    // Device custom-token bootstrap may complete just after the final
                                    // BLE status. Retry only the bounded, idempotent callable claim.
                                    var lastError: String? = null
                                    var claimed = false
                                    repeat(30) {
                                        if (claimed) return@repeat
                                        when (val result = cloudStore.claimDevice(deviceId, claimToken)) {
                                            is OwnershipClaimResult.Claimed -> {
                                                lastError = null
                                                claimed = true
                                            }
                                            is OwnershipClaimResult.Retryable -> {
                                                lastError = result.code
                                                delay(1000)
                                            }
                                            is OwnershipClaimResult.Rejected -> {
                                                throw IllegalStateException(claimFailureMessage(result.code))
                                            }
                                        }
                                    }
                                    if (!claimed) throw IllegalStateException(claimFailureMessage(lastError ?: "CLAIM_UNAVAILABLE"))
                                    _provisioningState.value = ProvisioningState.Success(deviceId)
                                } catch (e: Exception) {
                                    _provisioningState.value = ProvisioningState.Error("Failed to claim device: ${e.message}")
                                }
                            }
                        } else {
                            _provisioningState.value = ProvisioningState.Error("Sign in with Google or a verified email before claiming this device")
                        }
                    } else {
                        // capitalize the first letter for display
                        val displayStatus = status.replaceFirstChar { if (it.isLowerCase()) it.titlecase() else it.toString() }
                        _provisioningState.value = ProvisioningState.ProvisioningUpdate(displayStatus)
                    }
                } else if (type == "error") {
                    _provisioningState.value = ProvisioningState.Error(json.optString("message", "Provisioning Error"))
                }
            }, { error ->
                _provisioningState.value = ProvisioningState.Error("Provisioning failed: ${error.message}")
            })
    }

    fun claimOwnershipPairing(macAddress: String) {
        if (AccountSession.state(FirebaseAuth.getInstance().currentUser) != DurableAccountState.ELIGIBLE) {
            _provisioningState.value = ProvisioningState.Error("Sign in with Google or a verified email before claiming this device")
            return
        }
        _provisioningState.value = ProvisioningState.OwnershipPairing
        provisionDisposable?.dispose()
        provisionDisposable = bleClient.retrieveOwnershipPairingProof(macAddress)
            .subscribeOn(Schedulers.io())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe({ json ->
                when (json.optString("type")) {
                    "token" -> {
                        val proof = json.optString("value")
                        val deviceId = json.optString("device_id")
                        if (proof.isEmpty() || deviceId.isEmpty()) {
                            _provisioningState.value = ProvisioningState.Error("The device did not provide a valid ownership pairing proof")
                            return@subscribe
                        }
                        provisionDisposable?.dispose()
                        claimNearbyDevice(deviceId, proof)
                    }
                    "error" -> {
                        provisionDisposable?.dispose()
                        _provisioningState.value = ProvisioningState.Error(json.optString("message", "Ownership pairing failed"))
                    }
                }
            }, { error ->
                _provisioningState.value = ProvisioningState.Error("Ownership pairing failed: ${error.message}")
            })
    }

    private fun claimNearbyDevice(deviceId: String, proof: String) {
        viewModelScope.launch {
            try {
                var lastError: String? = null
                repeat(30) {
                    when (val result = cloudStore.claimDevice(deviceId, proof)) {
                        is OwnershipClaimResult.Claimed -> {
                            _provisioningState.value = ProvisioningState.Success(deviceId)
                            return@launch
                        }
                        is OwnershipClaimResult.Retryable -> {
                            lastError = result.code
                            delay(1000)
                        }
                        is OwnershipClaimResult.Rejected -> {
                            throw IllegalStateException(claimFailureMessage(result.code))
                        }
                    }
                }
                throw IllegalStateException(claimFailureMessage(lastError ?: "CLAIM_UNAVAILABLE"))
            } catch (error: Exception) {
                _provisioningState.value = ProvisioningState.Error("Failed to claim device: ${error.message}")
            }
        }
    }

    override fun onCleared() {
        super.onCleared()
        scanDisposable?.dispose()
        provisionDisposable?.dispose()
        wifiScanDisposable?.dispose()
    }

    private fun claimFailureMessage(code: String): String = when (code) {
        "ALREADY_CLAIMED" -> "This device is already registered to another account."
        "EXPIRED_PAIRING" -> "The secure pairing window expired. Start provisioning again."
        "DURABLE_ACCOUNT_REQUIRED", "EMAIL_VERIFICATION_REQUIRED" -> "Sign in with Google or verify your email before claiming this device."
        "INVALID_PAIRING_PROOF" -> "The secure BLE pairing proof was rejected. Start provisioning again."
        else -> "The device is connecting to SmartFlow Cloud. Please retry."
    }
}

sealed class ProvisioningState {
    object Idle : ProvisioningState()
    object Scanning : ProvisioningState()
    data class DeviceFound(val macAddress: String) : ProvisioningState()
    object ScanningWifi : ProvisioningState()
    data class WifiListReceived(val macAddress: String, val networks: List<WifiNetwork>, val isScanning: Boolean) : ProvisioningState()
    object Provisioning : ProvisioningState()
    object OwnershipPairing : ProvisioningState()
    data class ProvisioningUpdate(val message: String) : ProvisioningState()
    data class Success(val claimToken: String) : ProvisioningState()
    data class Error(val message: String) : ProvisioningState()
}
