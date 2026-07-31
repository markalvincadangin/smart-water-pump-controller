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
import com.smartflow.data.dto.DeviceEventDto
import com.smartflow.data.dto.DeviceShadowDto
import com.smartflow.data.dto.TelemetryDto
import com.smartflow.data.dto.toDto
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.delay
import com.smartflow.data.dto.DeviceStatusDto

interface DeviceRepository {
    val telemetryFlow: StateFlow<Telemetry>
    val shadowFlow: StateFlow<DeviceShadow>
    val configFlow: StateFlow<DeviceConfig>
    val connectionFlow: StateFlow<ConnectionState>
    val eventsFlow: StateFlow<List<com.smartflow.domain.DeviceEvent>>

    suspend fun initializeAuth()
    fun updateDesiredState(desired: com.smartflow.domain.ShadowDesired)
    fun updateConfig(config: DeviceConfig)
    fun registerFcmToken(token: String)
}

class FirebaseDeviceRepository(
    private val deviceId: String
) : DeviceRepository {

    private val database = FirebaseDatabase.getInstance()
    private val auth = FirebaseAuth.getInstance()
    private val deviceRef = database.getReference("devices").child(deviceId)

    private val repositoryScope = CoroutineScope(Dispatchers.Default)
    private var lastHeartbeatTimeMs: Long = 0L
    private var isAppConnectedToFirebase: Boolean = false

    private val _telemetryFlow = MutableStateFlow(Telemetry())
    override val telemetryFlow = _telemetryFlow.asStateFlow()

    private val _shadowFlow = MutableStateFlow(DeviceShadow())
    override val shadowFlow = _shadowFlow.asStateFlow()

    private val _configFlow = MutableStateFlow(DeviceConfig())
    override val configFlow = _configFlow.asStateFlow()

    private val _connectionFlow = MutableStateFlow(ConnectionState.DISCONNECTED)
    override val connectionFlow = _connectionFlow.asStateFlow()

    private val _eventsFlow = MutableStateFlow<List<com.smartflow.domain.DeviceEvent>>(emptyList())
    override val eventsFlow = _eventsFlow.asStateFlow()

    init {
        // Track Firebase connected state (.info/connected)
        database.getReference(".info/connected").addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                isAppConnectedToFirebase = snapshot.getValue(Boolean::class.java) ?: false
                if (!isAppConnectedToFirebase) {
                    _connectionFlow.value = ConnectionState.DISCONNECTED
                } else if (System.currentTimeMillis() - lastHeartbeatTimeMs <= 10000L) {
                    _connectionFlow.value = ConnectionState.CONNECTED
                }
            }
            override fun onCancelled(error: DatabaseError) {}
        })

        repositoryScope.launch {
            while (true) {
                delay(2000)
                if (isAppConnectedToFirebase) {
                    if (System.currentTimeMillis() - lastHeartbeatTimeMs > 10000L) {
                        _connectionFlow.value = ConnectionState.DISCONNECTED
                    } else {
                        _connectionFlow.value = ConnectionState.CONNECTED
                    }
                }
            }
        }
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
                val t = snapshot.getValue(TelemetryDto::class.java)
                if (t != null) {
                    _telemetryFlow.value = t.toDomain()
                }
            }
            override fun onCancelled(error: DatabaseError) {}
        })

        deviceRef.child("shadow").addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val s = snapshot.getValue(DeviceShadowDto::class.java)
                if (s != null) {
                    _shadowFlow.value = s.toDomain()
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

        deviceRef.child("events").orderByKey().limitToLast(20).addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val eventsList = mutableListOf<com.smartflow.domain.DeviceEvent>()
                for (child in snapshot.children) {
                    val event = child.getValue(DeviceEventDto::class.java)
                    if (event != null && child.key != null) {
                        eventsList.add(event.toDomain(child.key!!))
                    }
                }
                // Reverse the list to show newest first
                _eventsFlow.value = eventsList.reversed()
            }
            override fun onCancelled(error: DatabaseError) {}
        })

        deviceRef.child("status").addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                val s = snapshot.getValue(DeviceStatusDto::class.java)
                if (s != null) {
                    lastHeartbeatTimeMs = System.currentTimeMillis()
                    if (isAppConnectedToFirebase) {
                        if (s.lifecycle.equals("ONLINE", ignoreCase = true)) {
                            _connectionFlow.value = ConnectionState.CONNECTED
                        } else {
                            _connectionFlow.value = ConnectionState.DISCONNECTED
                        }
                    }
                } else {
                    if (isAppConnectedToFirebase) {
                        _connectionFlow.value = ConnectionState.DISCONNECTED
                    }
                }
            }
            override fun onCancelled(error: DatabaseError) {}
        })
    }

    override fun updateDesiredState(desired: com.smartflow.domain.ShadowDesired) {
        if (_connectionFlow.value == ConnectionState.CONNECTED) {
            deviceRef.child("shadow/desired").setValue(desired.toDto())
        }
    }

    override fun updateConfig(config: DeviceConfig) {
        if (_connectionFlow.value == ConnectionState.CONNECTED) {
            deviceRef.child("settings").setValue(config)
        }
    }

    override fun registerFcmToken(token: String) {
        deviceRef.child("fcmTokens").child(token.hashCode().toString()).setValue(token)
    }
}
