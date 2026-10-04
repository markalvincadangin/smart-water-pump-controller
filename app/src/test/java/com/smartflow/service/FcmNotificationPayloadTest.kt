package com.smartflow.service

import com.google.firebase.messaging.RemoteMessage
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertNotNull
import org.junit.Test

class FcmNotificationPayloadTest {

    @Test
    fun parsesCanonicalEventData() {
        val payload = FcmNotificationPayload.from(
            data = mapOf(
                "title" to "Maximum Runtime Protection",
                "body" to "Maximum pump runtime was exceeded.",
                "tag" to "maxRuntime",
                "eventCode" to "EVT_MAX_RUNTIME_EXCEEDED",
                "eventId" to "evt-1",
                "deviceId" to "smart-flow-01"
            )
        )

        assertNotNull(payload)
        assertEquals("Maximum Runtime Protection", payload?.title)
        assertEquals("Maximum pump runtime was exceeded.", payload?.body)
        assertEquals("maxRuntime", payload?.tag)
        assertEquals("EVT_MAX_RUNTIME_EXCEEDED", payload?.eventCode)
        assertEquals("evt-1", payload?.eventId)
        assertEquals("smart-flow-01", payload?.deviceId)
    }

    @Test
    fun parsesDerivedNotificationData() {
        val payload = FcmNotificationPayload.from(
            data = mapOf(
                "title" to "Pump Started",
                "body" to "Tank: 80%, Flow: 4.2 LPM",
                "tag" to "pumpStarted"
            )
        )

        assertEquals("Pump Started", payload?.title)
        assertEquals("Tank: 80%, Flow: 4.2 LPM", payload?.body)
        assertEquals("pumpStarted", payload?.tag)
    }

    @Test
    fun parsesNotificationPayloadOverData() {
        val payload = FcmNotificationPayload.from(
            notificationTitle = "Notification Title",
            notificationBody = "Notification Body",
            data = mapOf(
                "title" to "Data Title",
                "body" to "Data Body",
                "tag" to "lowLevel"
            )
        )

        assertNotNull(payload)
        assertEquals("Notification Title", payload?.title)
        assertEquals("Notification Body", payload?.body)
        assertEquals("lowLevel", payload?.tag)
    }

    @Test
    fun rejectsMessageWithoutTitleOrBody() {
        val payload = FcmNotificationPayload.from(data = mapOf("tag" to "lowLevel"))

        assertNull(payload)
    }
}
