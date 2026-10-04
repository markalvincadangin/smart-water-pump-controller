package com.smartflow.data.dto

import com.google.firebase.database.PropertyName
import com.smartflow.domain.DeviceConfig
import com.smartflow.domain.OperatingMode
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class FirebaseModelsTest {

    @Test
    fun deviceConfigMapsToDtoUsingCanonicalValues() {
        val domain = DeviceConfig(
            pumpStartLevelPct = 25,
            pumpStopLevelPct = 90,
            dryRunThresholdLpm = 1.5f,
            maxPumpRuntimeMin = 60
        )

        assertEquals(domain, domain.toDto().toDomain())
    }

    @Test
    fun deviceConfigDtoUsesCanonicalFirebaseFieldNames() {
        assertPropertyName(DeviceConfigDto::class.java, "getPumpStartLevelPct", "pump_start_level_pct")
        assertPropertyName(DeviceConfigDto::class.java, "setPumpStartLevelPct", "pump_start_level_pct", Int::class.java)
        assertPropertyName(DeviceConfigDto::class.java, "getPumpStopLevelPct", "pump_stop_level_pct")
        assertPropertyName(DeviceConfigDto::class.java, "setPumpStopLevelPct", "pump_stop_level_pct", Int::class.java)
        assertPropertyName(DeviceConfigDto::class.java, "getDryRunThresholdLpm", "dry_run_threshold_lpm")
        assertPropertyName(DeviceConfigDto::class.java, "setDryRunThresholdLpm", "dry_run_threshold_lpm", Float::class.java)
        assertPropertyName(DeviceConfigDto::class.java, "getMaxPumpRuntimeMin", "max_pump_runtime_min")
        assertPropertyName(DeviceConfigDto::class.java, "setMaxPumpRuntimeMin", "max_pump_runtime_min", Int::class.java)
    }

    @Test
    fun shadowReportedMapsLastFaultCodeToDomain() {
        val dto = ShadowReportedDto(
            runMode = "MANUAL_OFF",
            isRunning = false,
            isError = true,
            lastFaultMessage = "Maximum pump runtime exceeded.",
            lastFaultCode = "MAX_RUNTIME"
        )

        val domain = dto.toDomain()

        assertEquals("MAX_RUNTIME", domain.lastFaultCode)
        assertEquals(dto.lastFaultMessage, domain.lastFaultMessage)
        assertEquals(dto.runMode, domain.runMode)
    }

    @Test
    fun shadowReportedUsesCanonicalLastFaultCodeFieldName() {
        assertPropertyName(ShadowReportedDto::class.java, "getLastFaultCode", "last_fault_code")
        assertPropertyName(ShadowReportedDto::class.java, "setLastFaultCode", "last_fault_code", String::class.java)
    }

    @Test
    fun shadowDesiredDefaultsAreSafe() {
        val dto = ShadowDesiredDto()
        val domain = dto.toDomain()

        assertEquals(OperatingMode.MANUAL.name, dto.mode)
        assertFalse(dto.bypassLevelSensor)
        assertFalse(dto.bypassFlowSensor)
        assertEquals(OperatingMode.MANUAL.name, domain.mode)
        assertFalse(domain.bypassLevelSensor)
        assertFalse(domain.bypassFlowSensor)
    }

    @Test
    fun shadowDesiredRoundTripsAllContractFields() {
        val domain = com.smartflow.domain.ShadowDesired(
            mode = OperatingMode.COUNTDOWN.name,
            manualDesired = true,
            countdownStart = true,
            countdownDurationMin = 45,
            emergencyStop = false,
            resetStop = false,
            clearError = false,
            bypassLevelSensor = false,
            bypassFlowSensor = true,
            rebootDevice = false
        )

        assertEquals(domain, domain.toDto().toDomain())
    }

    @Test
    fun shadowReportedDtoRoundTripsAllContractFields() {
        val dto = ShadowReportedDto(
            runMode = "COUNTDOWN",
            isRunning = true,
            countdownRemainingSec = 90,
            isError = false,
            isOverflowError = false,
            emergencyStopLatched = false,
            lastFaultMessage = "",
            lastFaultCode = ""
        )

        val domain = dto.toDomain()

        assertEquals(dto.runMode, domain.runMode)
        assertEquals(dto.isRunning, domain.isRunning)
        assertEquals(dto.countdownRemainingSec, domain.countdownRemainingSec)
        assertEquals(dto.isError, domain.isError)
        assertEquals(dto.isOverflowError, domain.isOverflowError)
        assertEquals(dto.emergencyStopLatched, domain.emergencyStopLatched)
        assertEquals(dto.lastFaultMessage, domain.lastFaultMessage)
        assertEquals(dto.lastFaultCode, domain.lastFaultCode)
    }

    @Test
    fun deviceShadowDtoUsesSafeDefaults() {
        val dto = DeviceShadowDto()
        assertEquals(OperatingMode.MANUAL.name, dto.desired.mode)
        assertFalse(dto.desired.bypassLevelSensor)
        assertFalse(dto.desired.bypassFlowSensor)
        assertEquals("", dto.reported.lastFaultCode)
    }

    private fun assertPropertyName(
        dtoClass: Class<*>,
        methodName: String,
        expected: String,
        vararg parameterTypes: Class<*>
    ) {
        val method = dtoClass.getDeclaredMethod(methodName, *parameterTypes)
        val annotation = method.getAnnotation(PropertyName::class.java)
        assertTrue("Missing @PropertyName on $methodName", annotation != null)
        assertEquals(expected, annotation.value)
    }
}