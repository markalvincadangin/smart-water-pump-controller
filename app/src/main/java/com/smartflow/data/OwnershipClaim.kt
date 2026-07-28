package com.smartflow.data

/**
 * Stable outcomes returned by the ownership claim boundary.  The UI must never
 * infer ownership from a direct RTDB write or from a generic exception string.
 */
sealed interface OwnershipClaimResult {
    data class Claimed(val deviceId: String, val auditId: String?) : OwnershipClaimResult
    data class Retryable(val code: String) : OwnershipClaimResult
    data class Rejected(val code: String) : OwnershipClaimResult
}

enum class DurableAccountState {
    SIGNED_OUT,
    ANONYMOUS,
    UNVERIFIED_EMAIL,
    ELIGIBLE
}
