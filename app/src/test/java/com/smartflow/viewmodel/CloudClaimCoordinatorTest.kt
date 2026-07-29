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
            add(OwnershipClaimResult.Claimed("SF-67D42C", "audit-1"))
        }
        val attempts = mutableListOf<Int>()
        val coordinator = CloudClaimCoordinator(
            claim = { _, _ -> responses.removeFirst() },
            maxAttempts = 45,
            retryDelayMs = 2_000,
            wait = { }
        )

        val outcome = coordinator.claimWhenReady("SF-67D42C", "proof") { attempt, _ -> attempts += attempt }

        assertEquals(CloudClaimOutcome.Claimed("SF-67D42C"), outcome)
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

        val outcome = coordinator.claimWhenReady("SF-67D42C", "proof") { attempt, _ -> attempts += attempt }

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

        val outcome = coordinator.claimWhenReady("SF-67D42C", "proof") { _, _ -> }

        assertEquals(CloudClaimOutcome.Rejected("EXPIRED_PAIRING"), outcome)
        assertTrue(calls == 1)
    }
}
