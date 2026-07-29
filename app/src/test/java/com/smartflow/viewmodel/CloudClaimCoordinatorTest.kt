package com.smartflow.viewmodel

import com.smartflow.data.OwnershipClaimResult
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class CloudClaimCoordinatorTest {
    @Test
    fun `retries unavailable claim and reports progress until claimed`() = runTest {
        val responses = ArrayDeque<OwnershipClaimResult>().apply {
            add(OwnershipClaimResult.Retryable("CLAIM_UNAVAILABLE"))
            add(OwnershipClaimResult.Claimed("SF-TEST01", "audit-1"))
        }
        val attempts = mutableListOf<Int>()
        val coordinator = CloudClaimCoordinator(
            claim = { _, _ -> responses.removeFirst() },
            maxAttempts = 45,
            retryDelayMs = 2_000,
            wait = { }
        )

        val outcome = coordinator.claimWhenReady("SF-TEST01", "proof") { attempt, _ -> attempts += attempt }

        assertEquals(CloudClaimOutcome.Claimed("SF-TEST01"), outcome)
        assertEquals(listOf(1, 2), attempts)
    }

    @Test
    fun `times out after bounded retryable attempts`() = runTest {
        val attempts = mutableListOf<Int>()
        val coordinator = CloudClaimCoordinator(
            claim = { _, _ -> OwnershipClaimResult.Retryable("CLAIM_UNAVAILABLE") },
            maxAttempts = 3,
            retryDelayMs = 1,
            wait = { }
        )

        val outcome = coordinator.claimWhenReady("SF-TEST01", "proof") { attempt, _ -> attempts += attempt }

        assertEquals(CloudClaimOutcome.TimedOut("CLAIM_UNAVAILABLE"), outcome)
        assertEquals(listOf(1, 2, 3), attempts)
    }

    @Test
    fun `rejects immediately without a retry`() = runTest {
        var calls = 0
        val coordinator = CloudClaimCoordinator(
            claim = { _, _ ->
                calls += 1
                OwnershipClaimResult.Rejected("EXPIRED_PAIRING")
            },
            maxAttempts = 45,
            retryDelayMs = 1,
            wait = { error("Rejected outcomes must not wait") }
        )

        val outcome = coordinator.claimWhenReady("SF-TEST01", "proof") { _, _ -> }

        assertEquals(CloudClaimOutcome.Rejected("EXPIRED_PAIRING"), outcome)
        assertTrue(calls == 1)
    }
}
