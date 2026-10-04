package com.smartflow.service

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class NotificationRouteTest {

    @Test
    fun parsesCanonicalRoutingExtras() {
        val route = notificationRouteTarget(
            isSmartflowNotification = true,
            deviceId = "device-01",
            eventId = "event-01",
            eventCode = "EVT_MAX_RUNTIME_EXCEEDED",
            tag = "maxRuntime"
        )

        assertEquals("device-01", route?.deviceId)
        assertEquals("event-01", route?.eventId)
        assertEquals("EVT_MAX_RUNTIME_EXCEEDED", route?.eventCode)
        assertEquals("maxRuntime", route?.tag)
    }

    @Test
    fun rejectsUnmarkedIntent() {
        assertNull(notificationRouteTarget(isSmartflowNotification = false))
    }

    @Test
    fun rejectsMarkedIntentWithoutRoutingContext() {
        assertNull(notificationRouteTarget(isSmartflowNotification = true))
    }
}
