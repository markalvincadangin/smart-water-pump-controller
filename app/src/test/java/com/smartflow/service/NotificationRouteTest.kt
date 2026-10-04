package com.smartflow.service

import android.content.Intent
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class NotificationRouteTest {

    @Test
    fun parsesCanonicalRoutingExtras() {
        val intent = Intent().apply {
            putExtra("smartflow_notification", true)
            putExtra("deviceId", "device-01")
            putExtra("eventId", "event-01")
            putExtra("eventCode", "EVT_MAX_RUNTIME_EXCEEDED")
            putExtra("tag", "maxRuntime")
        }

        val route = notificationRouteTarget(intent)

        assertEquals("device-01", route?.deviceId)
        assertEquals("event-01", route?.eventId)
        assertEquals("EVT_MAX_RUNTIME_EXCEEDED", route?.eventCode)
        assertEquals("maxRuntime", route?.tag)
    }

    @Test
    fun rejectsUnmarkedIntent() {
        assertNull(notificationRouteTarget(Intent()))
    }

    @Test
    fun rejectsMarkedIntentWithoutRoutingContext() {
        val intent = Intent().apply {
            putExtra("smartflow_notification", true)
        }

        assertNull(notificationRouteTarget(intent))
    }
}
