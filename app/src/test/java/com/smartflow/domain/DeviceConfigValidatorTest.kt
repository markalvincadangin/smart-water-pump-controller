package com.smartflow.domain

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class DeviceConfigValidatorTest {

    @Test
    fun defaultConfigurationIsValid() {
        assertTrue(DeviceConfigValidator.validate(DeviceConfig()).isValid)
    }

    @Test
    fun pumpLevelsAcceptContractBoundariesWhenOrdered() {
        assertTrue(
            DeviceConfigValidator.validate(
                DeviceConfig(
                    pumpStartLevelPct = 0,
                    pumpStopLevelPct = 1
                )
            ).isValid
        )

        assertTrue(
            DeviceConfigValidator.validate(
                DeviceConfig(
                    pumpStartLevelPct = 99,
                    pumpStopLevelPct = 100
                )
            ).isValid
        )
    }

    @Test
    fun pumpStopLevelMustBeGreaterThanStartLevel() {
        assertFalse(
            DeviceConfigValidator.validate(
                DeviceConfig(
                    pumpStartLevelPct = 50,
                    pumpStopLevelPct = 50
                )
            ).isValid
        )

        assertFalse(
            DeviceConfigValidator.validate(
                DeviceConfig(
                    pumpStartLevelPct = 75,
                    pumpStopLevelPct = 50
                )
            ).isValid
        )
    }

    @Test
    fun pumpLevelsRejectValuesOutsideZeroToOneHundredPercent() {
        assertFalse(
            DeviceConfigValidator.validate(
                DeviceConfig(
                    pumpStartLevelPct = -1,
                    pumpStopLevelPct = 50
                )
            ).isValid
        )

        assertFalse(
            DeviceConfigValidator.validate(
                DeviceConfig(
                    pumpStartLevelPct = 50,
                    pumpStopLevelPct = 101
                )
            ).isValid
        )
    }

    @Test
    fun dryRunThresholdAcceptsContractBoundaries() {
        assertTrue(
            DeviceConfigValidator.validate(
                DeviceConfig(dryRunThresholdLpm = 0.1f)
            ).isValid
        )
        assertTrue(
            DeviceConfigValidator.validate(
                DeviceConfig(dryRunThresholdLpm = 10.0f)
            ).isValid
        )
    }

    @Test
    fun dryRunThresholdRejectsValuesOutsideContractRange() {
        assertFalse(
            DeviceConfigValidator.validate(
                DeviceConfig(dryRunThresholdLpm = 0.09f)
            ).isValid
        )
        assertFalse(
            DeviceConfigValidator.validate(
                DeviceConfig(dryRunThresholdLpm = 10.01f)
            ).isValid
        )
    }

    @Test
    fun maximumPumpRuntimeAcceptsContractBoundaries() {
        assertTrue(
            DeviceConfigValidator.validate(
                DeviceConfig(maxPumpRuntimeMin = 30)
            ).isValid
        )
        assertTrue(
            DeviceConfigValidator.validate(
                DeviceConfig(maxPumpRuntimeMin = 120)
            ).isValid
        )
    }

    @Test
    fun maximumPumpRuntimeRejectsValuesOutsideContractRange() {
        assertFalse(
            DeviceConfigValidator.validate(
                DeviceConfig(maxPumpRuntimeMin = 29)
            ).isValid
        )
        assertFalse(
            DeviceConfigValidator.validate(
                DeviceConfig(maxPumpRuntimeMin = 121)
            ).isValid
        )
    }

    @Test
    fun countdownAcceptsContractBoundaries() {
        assertTrue(
            DeviceConfigValidator.validateCountdownDuration(1) == null
        )
        assertTrue(
            DeviceConfigValidator.validateCountdownDuration(120) == null
        )
    }

    @Test
    fun countdownRejectsValuesOutsideContractRange() {
        assertTrue(
            DeviceConfigValidator.validateCountdownDuration(0) != null
        )
        assertTrue(
            DeviceConfigValidator.validateCountdownDuration(121) != null
        )
    }
}
