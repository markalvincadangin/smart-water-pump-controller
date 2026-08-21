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
            title = "Pump started",
            logMessage = "Command executed successfully.",
            notificationMessage = "Your water pump has been turned on."
        ),
        "EVT_PUMP_OFF" to EventDefinition(
            category = "Pump Controller",
            title = "Pump stopped",
            logMessage = "Command executed successfully.",
            notificationMessage = "Your water pump has been turned off."
        ),
        "EVT_SENSOR_LEVEL_FAIL" to EventDefinition(
            category = "Sensors",
            title = "Level sensor offline",
            logMessage = "Sensor failed to respond after multiple retries.",
            notificationMessage = "We lost connection to your water level sensor. Please check if it's obstructed."
        ),
        "EVT_SENSOR_LEVEL_RECOVERED" to EventDefinition(
            category = "Sensors",
            title = "Level sensor online",
            logMessage = "Communication restored.",
            notificationMessage = "Connection to your water level sensor has been restored."
        ),
        "EVT_SENSOR_FLOW_STUCK" to EventDefinition(
            category = "Sensors",
            title = "Abnormal flow detected",
            logMessage = "Continuous movement detected while pump is off.",
            notificationMessage = "Water is flowing unexpectedly while the pump is off. Please check for leaks!"
        ),
        "EVT_SENSOR_FLOW_RECOVERED" to EventDefinition(
            category = "Sensors",
            title = "Flow normal",
            logMessage = "Readings returned to normal boundaries.",
            notificationMessage = "Water flow readings are back to normal."
        ),
        "EVT_DRY_RUN_WARN" to EventDefinition(
            category = "Safety System",
            title = "Dry run warning",
            logMessage = "Low flow detected. Verifying...",
            notificationMessage = "Warning: The pump might be running dry. Verifying flow..."
        ),
        "EVT_DRY_RUN_LOCKOUT" to EventDefinition(
            category = "Safety System",
            title = "Dry run detected",
            logMessage = "Pump stopped to prevent damage.",
            notificationMessage = "Dry run detected! The pump was stopped automatically to prevent damage."
        ),
        "EVT_DRY_RUN_CLEARED" to EventDefinition(
            category = "Safety System",
            title = "Dry run cleared",
            logMessage = "Flow restored. Lockout cleared.",
            notificationMessage = "Water flow is back to normal. The dry-run warning was cleared."
        ),
        "EVT_MAX_RUNTIME_EXCEEDED" to EventDefinition(
            category = "Safety System",
            title = "Max runtime exceeded",
            logMessage = "Automatic shutdown initiated.",
            notificationMessage = "The pump ran longer than the safety limit and was stopped automatically."
        ),
        "EVT_FAIL_SAFE_STOP" to EventDefinition(
            category = "Safety System",
            title = "Fail-safe stop",
            logMessage = "Pump stopped due to unstable sensor data.",
            notificationMessage = "The pump was stopped as a precaution due to unstable sensor data."
        ),
        "EVT_AUTO_BYPASS_ENABLED" to EventDefinition(
            category = "Safety System",
            title = "Auto-bypass enabled",
            logMessage = "Sensor automatically bypassed due to sustained failure.",
            notificationMessage = "A sensor has failed. The system has automatically bypassed it to keep the pump running."
        ),
        "EVT_RS485_TIMEOUT" to EventDefinition(
            category = "Sensor Network",
            title = "Sensor network timeout",
            logMessage = "Remote nodes unreachable.",
            notificationMessage = "Lost communication with the remote sensors. Please check the wiring."
        ),
        "EVT_RS485_INVALID" to EventDefinition(
            category = "Sensor Network",
            title = "Sensor network error",
            logMessage = "Received invalid frame structure.",
            notificationMessage = "Received corrupted data from remote sensors."
        ),
        "EVT_WIFI_DISCONNECTED" to EventDefinition(
            category = "System",
            title = "Wi-Fi disconnected",
            logMessage = "Controller lost local network connection.",
            notificationMessage = "The smart controller has lost its Wi-Fi connection."
        ),
        "EVT_CRASH_LOOP_SAFE_MODE" to EventDefinition(
            category = "System",
            title = "Safe mode activated",
            logMessage = "Crash-loop detected. Booting without sensors.",
            notificationMessage = "The controller experienced multiple crashes and entered Safe Mode. Please update the firmware."
        ),
        "EVT_BOOT" to EventDefinition(
            category = "System",
            title = "System boot",
            logMessage = "Controller initialized.",
            notificationMessage = "The smart controller has successfully powered on."
        ),
        "EVT_CONFIG_RESTORED" to EventDefinition(
            category = "System",
            title = "Config restored",
            logMessage = "Loaded from Non-Volatile Storage.",
            notificationMessage = "Your previous pump settings have been restored."
        )
    )

    fun getEventDefinition(code: String): EventDefinition? {
        return registry[code]
    }
}
