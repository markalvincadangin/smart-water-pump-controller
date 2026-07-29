package com.smartflow.presentation

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.launch

/**
 * The app deliberately checks deletion eligibility before attempting any
 * Firebase Auth account-delete operation. Ownership remains authoritative in
 * the Cloud Function, rather than being inferred from the app's device list.
 */
@Composable
fun AccountManagementScreen(
    accountLabel: String,
    onCheckDeletionEligibility: suspend () -> Pair<Boolean, Int>,
    onBack: () -> Unit,
) {
    var busy by remember { mutableStateOf(false) }
    var message by remember { mutableStateOf("") }
    val scope = rememberCoroutineScope()

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.Center,
    ) {
        Text("Account", style = MaterialTheme.typography.headlineSmall)
        Spacer(Modifier.height(12.dp))
        Text(accountLabel, style = MaterialTheme.typography.bodyMedium)
        Spacer(Modifier.height(28.dp))
        Text("Delete account", style = MaterialTheme.typography.titleMedium)
        Spacer(Modifier.height(8.dp))
        Text(
            "Account deletion is unavailable while you own SmartFlow devices. " +
                "Release or transfer every device first.",
            style = MaterialTheme.typography.bodyMedium,
        )
        Spacer(Modifier.height(12.dp))
        Button(
            enabled = !busy,
            onClick = {
                busy = true
                scope.launch {
                    try {
                        val (eligible, ownedDeviceCount) = onCheckDeletionEligibility()
                        message = if (eligible) {
                            "No owned devices were found. Account deletion also requires recent sign-in; this final confirmation flow is not available in this build."
                        } else {
                            "Account deletion is blocked: release or transfer your $ownedDeviceCount owned device${if (ownedDeviceCount == 1) "" else "s"} first."
                        }
                    } catch (error: Exception) {
                        message = error.localizedMessage ?: "Unable to check account deletion eligibility."
                    } finally {
                        busy = false
                    }
                }
            },
            modifier = Modifier.fillMaxWidth(),
        ) { Text(if (busy) "Checking…" else "Check deletion eligibility") }
        if (message.isNotBlank()) {
            Spacer(Modifier.height(16.dp))
            Text(message, style = MaterialTheme.typography.bodyMedium)
        }
        Spacer(Modifier.height(28.dp))
        OutlinedButton(onClick = onBack, modifier = Modifier.fillMaxWidth()) { Text("Back to devices") }
    }
}
