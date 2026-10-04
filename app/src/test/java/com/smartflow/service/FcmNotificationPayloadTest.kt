package com.smartflow.service

import com.google.firebase.messaging.RemoteMessage
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertNotNull
import org.junit.Test

class FcmNotificationPayloadTest {

    @Test
    fun parsesCanonicalEventData() {
        val message = RemoteMessage.Builder("test@smartflow")
            .setData(
                mapOf(
                    "title" to "Maximum Runtime Protection",
                    "body" to "Maximum pump runtime was exceeded.",
                    "tag" to "maxRuntime",
                    "eventCode" to "EVT_MAX_RUNTIME_EXCEEDED",
                    "eventId" to "evt-1",
                    "deviceId" to "smart-flow-01"
                )
            )
            .build()

        val payload = FcmNotificationPayload.from(message)

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
        val message = RemoteMessage.Builder("test@smartflow")
            .setData(
                mapOf(
                    "title" to "Pump Started",
                    "body" to "Tank: 80%, Flow: 4.2 LPM",
                    "tag" to "pumpStarted"
                )
            )
            .build()

        val payload = FcmNotificationPayload.from(message)

        assertEquals("Pump Started", payload?.title)
        assertEquals("Tank: 80%, Flow: 4.2 LPM", payload?.body)
        assertEquals("pumpStarted", payload?.tag)
    }

    @Test
    fun rejectsMessageWithoutTitleOrBody() {
        val message = RemoteMessage.Builder("test@smartflow")
            .setData(mapOf("tag" to "lowLevel"))
            .build()

        assertNull(FcmNotificationPayload.from(message))
    }
}
