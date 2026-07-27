package com.smartflow.data

data class Device(
    val id: String = "",
    val metadata: DeviceMetadata? = null,
    val status: DeviceStatus? = null,
    val telemetry: DeviceTelemetry? = null,
    val settings: DeviceSettings? = null,
    val shadow: DeviceShadow? = null,
    val diagnostics: DeviceDiagnostics? = null
)

data class DeviceMetadata(
    val firmwareVersion: String = "",
    val hardwareVersion: String = "",
    val protocolVersion: String = "",
    val serialNumber: String = ""
)

data class DeviceStatus(
    val lifecycle: String = "",
    val uptimeSeconds: Long = 0
)

data class DeviceTelemetry(
    val waterLevel: Double = 0.0,
    val flowRate: Double = 0.0
)

data class DeviceSettings(
    val configVersion: Int = 0,
    val tankHeight: Int = 0,
    val lowThreshold: Int = 0
)

data class DeviceShadow(
    val desired: ShadowState? = null,
    val reported: ShadowState? = null
)

data class ShadowState(
    val pumpState: Boolean = false,
    val mode: String = ""
)

data class DeviceDiagnostics(
    val freeHeap: Long = 0,
    val wifiRSSI: Int = 0,
    val restartReason: String = ""
)

data class DeviceEvent(
    val id: String = "",
    val timestamp: Long = 0,
    val severity: String = "",
    val category: String = "",
    val code: String = "",
    val message: String = ""
)
