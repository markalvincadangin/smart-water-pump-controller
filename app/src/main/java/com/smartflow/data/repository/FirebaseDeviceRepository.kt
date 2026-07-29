package com.smartflow.data.repository

import android.util.Log
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import com.smartflow.domain.ConnectionState
import com.smartflow.domain.DeviceConfig
import com.smartflow.domain.DeviceShadow
import com.smartflow.domain.Telemetry
import com.smartflow.data.AccountSession
import com.smartflow.data.DurableAccountState
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

interface DeviceRepository {
    val telemetryFlow: StateFlow<Telemetry>
    val shadowFlow: StateFlow<DeviceShadow>
    val configFlow: StateFlow<DeviceConfig>
    val connectionFlow: StateFlow<ConnectionState>

    suspend fun initializeAuth()
    fun updateDesiredState(desired: com.smartflow.domain.ShadowDesired)
    fun updateConfig(config: DeviceConfig)
}

class FirebaseDeviceRepository(
    private val deviceId: String
) : DeviceRepository {

    private val database = FirebaseDatabase.getInstance()
    private val auth = FirebaseAuth.getInstance()
    private val deviceRef = database.getReference("devices").child(deviceId)

    private val _telemetryFlow = MutableStateFlow(Telemetry())
    override val telemetryFlow = _telemetryFlow.asStateFlow()

    private val _shadowFlow = MutableStateFlow(DeviceShadow())
    override val shadowFlow = _shadowFlow.asStateFlow()

    private val _configFlow = MutableStateFlow(DeviceConfig())
    override val configFlow = _configFlow.asStateFlow()

    private val _connectionFlow = MutableStateFlow(ConnectionState.DISCONNECTED)
    override val connectionFlow = _connectionFlow.asStateFlow()

    init {
        // Track Firebase connected state (.info/connected)
        database.getReference(".info/connected").addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val connected = snapshot.getValue(Boolean::class.java) ?: false
                if (connected) {
                    _connectionFlow.value = ConnectionState.CONNECTED
                } else {
                    _connectionFlow.value = ConnectionState.DISCONNECTED
                }
            }
            override fun onCancelled(error: DatabaseError) {}
        })
    }

    override suspend fun initializeAuth() {
        try {
            _connectionFlow.value = ConnectionState.CONNECTING
            if (AccountSession.refreshDurableState(auth) != DurableAccountState.ELIGIBLE) {
                Log.w("FirebaseDeviceRepository", "A durable account is required before device access")
                _connectionFlow.value = ConnectionState.DISCONNECTED
                return
            }
            startObserving()
        } catch (e: Exception) {
            Log.e("FirebaseDeviceRepository", "Auth failed", e)
            _connectionFlow.value = ConnectionState.DISCONNECTED
        }
    }

    private fun startObserving() {
        deviceRef.child("telemetry").addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val t = snapshot.getValue(Telemetry::class.java)
                if (t != null) {
                    _telemetryFlow.value = t
                }
            }
            override fun onCancelled(error: DatabaseError) {}
        })

        deviceRef.child("shadow").addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val s = snapshot.getValue(DeviceShadow::class.java)
                if (s != null) {
                    _shadowFlow.value = s
                }
            }
            override fun onCancelled(error: DatabaseError) {}
        })

        deviceRef.child("settings").addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val c = snapshot.getValue(DeviceConfig::class.java)
                if (c != null) {
                    _configFlow.value = c
                }
            }
            override fun onCancelled(error: DatabaseError) {}
        })
    }

    override fun updateDesiredState(desired: com.smartflow.domain.ShadowDesired) {
        if (_connectionFlow.value == ConnectionState.CONNECTED) {
            deviceRef.child("shadow/desired").setValue(desired)
        }
    }

    override fun updateConfig(config: DeviceConfig) {
        if (_connectionFlow.value == ConnectionState.CONNECTED) {
            deviceRef.child("settings").setValue(config)
        }
    }
}
