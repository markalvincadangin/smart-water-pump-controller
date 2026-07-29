package com.smartflow.viewmodel

import com.smartflow.data.OwnershipClaimResult
import kotlinx.coroutines.delay

/**
 * Owns the bounded post-BLE cloud handoff. The caller keeps any pairing proof
 * in memory and supplies the atomic cloud-claim operation; this class never
 * persists or logs the proof.
 */
class CloudClaimCoordinator(
    private val claim: suspend (deviceId: String, pairingProof: String) -> OwnershipClaimResult,
    private val maxAttempts: Int = DEFAULT_MAX_ATTEMPTS,
    private val retryDelayMs: Long = DEFAULT_RETRY_DELAY_MS,
    private val wait: suspend (Long) -> Unit = { delay(it) }
) {
    suspend fun claimWhenReady(
        deviceId: String,
        pairingProof: String,
        onAttempt: (attempt: Int, maxAttempts: Int) -> Unit
    ): CloudClaimOutcome {
        var lastRetryableCode: String? = null
        repeat(maxAttempts) { index ->
            onAttempt(index + 1, maxAttempts)
            when (val result = claim(deviceId, pairingProof)) {
                is OwnershipClaimResult.Claimed -> return CloudClaimOutcome.Claimed(result.deviceId)
                is OwnershipClaimResult.Rejected -> return CloudClaimOutcome.Rejected(result.code)
                is OwnershipClaimResult.Retryable -> {
                    lastRetryableCode = result.code
                    // Keep the visible wait bounded at maxAttempts * retryDelayMs,
                    // including the final unavailable result.
                    wait(retryDelayMs)
                }
            }
        }
        return CloudClaimOutcome.TimedOut(lastRetryableCode ?: "CLAIM_UNAVAILABLE")
    }

    private companion object {
        const val DEFAULT_MAX_ATTEMPTS = 45
        const val DEFAULT_RETRY_DELAY_MS = 2_000L
    }
}

sealed class CloudClaimOutcome {
    data class Claimed(val deviceId: String) : CloudClaimOutcome()
    data class Rejected(val code: String) : CloudClaimOutcome()
    data class TimedOut(val lastCode: String) : CloudClaimOutcome()
}
