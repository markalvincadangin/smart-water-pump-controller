package com.smartflow.domain

object EventRegistry {
    data class EventDefinition(
        val category: String,
        val title: String,
        val logMessage: String,
        val notificationMessage: String
    )

    private val registry = mapOf(
        "EVT_PUMP_ON" to EventDefinition(
            category = "Pump Controller",
            title = "Pump Started",
            logMessage = "System Operation: Water pump has been energized and is currently active.",
            notificationMessage = "Your water pump has been turned on."
        ),
        "EVT_PUMP_OFF" to EventDefinition(
            category = "Pump Controller",
            title = "Pump Stopped",
            logMessage = "System Operation: Water pump has been de-energized and is currently idle.",
            notificationMessage = "Your water pump has been turned off."
        ),
        "EVT_SENSOR_LEVEL_FAIL" to EventDefinition(
            category = "Sensors",
            title = "Level Sensor Offline",
            logMessage = "Hardware Fault: Ultrasonic level sensor failed to respond after multiple retries.",
            notificationMessage = "We lost connection to your water level sensor. Please check if it's obstructed."
        ),
        "EVT_SENSOR_LEVEL_RECOVERED" to EventDefinition(
            category = "Sensors",
            title = "Level Sensor Online",
            logMessage = "Hardware Status: Ultrasonic level sensor communication restored.",
            notificationMessage = "Connection to your water level sensor has been restored."
        ),
        "EVT_SENSOR_FLOW_STUCK" to EventDefinition(
            category = "Sensors",
            title = "Abnormal Flow Detected",
            logMessage = "Hardware Fault: Flow sensor detects continuous movement while pump is off. Possible leak.",
            notificationMessage = "Water is flowing unexpectedly while the pump is off. Please check for leaks!"
        ),
        "EVT_SENSOR_FLOW_RECOVERED" to EventDefinition(
            category = "Sensors",
            title = "Flow Normal",
            logMessage = "Hardware Status: Flow sensor readings returned to normal boundaries.",
            notificationMessage = "Water flow readings are back to normal."
        ),
        "EVT_DRY_RUN_WARN" to EventDefinition(
            category = "Safety System",
            title = "Dry Run Warning",
            logMessage = "Safety Alert: Dry-run condition suspected. Initiating verification timer.",
            notificationMessage = "Warning: The pump might be running dry. Verifying flow..."
        ),
        "EVT_DRY_RUN_LOCKOUT" to EventDefinition(
            category = "Safety System",
            title = "Dry Run Lockout",
            logMessage = "Safety Lockout: Confirmed dry-run condition. Pump operations suspended to prevent damage.",
            notificationMessage = "Dry run detected! The pump was stopped automatically to prevent damage."
        ),
        "EVT_DRY_RUN_CLEARED" to EventDefinition(
            category = "Safety System",
            title = "Dry Run Cleared",
            logMessage = "Safety Status: Flow restored. Dry-run lockout cleared.",
            notificationMessage = "Water flow is back to normal. The dry-run warning was cleared."
        ),
        "EVT_MAX_RUNTIME_EXCEEDED" to EventDefinition(
            category = "Safety System",
            title = "Max Runtime Exceeded",
            logMessage = "Safety Lockout: Pump exceeded maximum continuous runtime. Automatic shutdown initiated.",
            notificationMessage = "The pump ran longer than the safety limit and was stopped automatically."
        ),
        "EVT_FAIL_SAFE_STOP" to EventDefinition(
            category = "Safety System",
            title = "Fail-Safe Stop",
            logMessage = "Safety Lockout: Insufficient stable data for autonomous operation. Pump stopped.",
            notificationMessage = "The pump was stopped as a precaution due to unstable sensor data."
        ),
        "EVT_AUTO_BYPASS_ENABLED" to EventDefinition(
            category = "Safety System",
            title = "Auto-Bypass Enabled",
            logMessage = "Safety Fallback: Sensor auto-bypass engaged due to sustained sensor failure.",
            notificationMessage = "A sensor has failed. The system has automatically bypassed it to keep the pump running."
        ),
        "EVT_RS485_TIMEOUT" to EventDefinition(
            category = "Sensor Network",
            title = "Sensor Network Timeout",
            logMessage = "Hardware Fault: RS485 communication timeout. Remote nodes unreachable.",
            notificationMessage = "Lost communication with the remote sensors. Please check the wiring."
        ),
        "EVT_RS485_INVALID" to EventDefinition(
            category = "Sensor Network",
            title = "Sensor Network Error",
            logMessage = "Hardware Fault: RS485 received invalid frame structure.",
            notificationMessage = "Received corrupted data from remote sensors."
        ),
        "EVT_WIFI_DISCONNECTED" to EventDefinition(
            category = "System",
            title = "Wi-Fi Disconnected",
            logMessage = "Network Status: Main controller lost connection to the local Wi-Fi network.",
            notificationMessage = "The smart controller has lost its Wi-Fi connection."
        ),
        "EVT_CRASH_LOOP_SAFE_MODE" to EventDefinition(
            category = "System",
            title = "Safe Mode Activated",
            logMessage = "System Alert: Device crash-loop detected. Booting into Safe Mode (sensors and Firebase disabled).",
            notificationMessage = "The controller experienced multiple crashes and entered Safe Mode. Please update the firmware."
        ),
        "EVT_BOOT" to EventDefinition(
            category = "System",
            title = "System Boot",
            logMessage = "System Status: Main controller initialized and entered main operational loop.",
            notificationMessage = "The smart controller has successfully powered on."
        ),
        "EVT_CONFIG_RESTORED" to EventDefinition(
            category = "System",
            title = "Config Restored",
            logMessage = "System Status: Configuration loaded from Non-Volatile Storage (NVS).",
            notificationMessage = "Your previous pump settings have been restored."
        )
    )

    fun getEventDefinition(code: String): EventDefinition? {
        return registry[code]
    }
}
