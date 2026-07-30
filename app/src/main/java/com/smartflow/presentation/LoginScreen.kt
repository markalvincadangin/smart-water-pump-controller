package com.smartflow.presentation

import android.app.Activity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import com.google.android.gms.auth.api.signin.GoogleSignIn
import com.google.android.gms.auth.api.signin.GoogleSignInOptions
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.GoogleAuthProvider
import kotlinx.coroutines.launch
import kotlinx.coroutines.tasks.await

@Composable
fun LoginScreen(onLoginSuccess: () -> Unit) {
    var email by remember { mutableStateOf("") }
    var password by remember { mutableStateOf("") }
    var error by remember { mutableStateOf("") }
    var isLoading by remember { mutableStateOf(false) }
    val coroutineScope = rememberCoroutineScope()
    val context = LocalContext.current
    val auth = FirebaseAuth.getInstance()
    val googleLauncher = rememberLauncherForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
        if (result.resultCode != Activity.RESULT_OK) return@rememberLauncherForActivityResult
        isLoading = true
        coroutineScope.launch {
            try {
                val account = GoogleSignIn.getSignedInAccountFromIntent(result.data).result
                val credential = GoogleAuthProvider.getCredential(account.idToken, null)
                auth.signInWithCredential(credential).await()
                onLoginSuccess()
            } catch (exception: Exception) {
                error = exception.localizedMessage ?: "Google sign-in failed"
                isLoading = false
            }
        }
    }

    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.Center,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        androidx.compose.foundation.Image(
            painter = androidx.compose.ui.res.painterResource(id = com.smartflow.R.drawable.logo_stacked),
            contentDescription = "SmartFlow Logo",
            modifier = Modifier.height(120.dp).fillMaxWidth().padding(horizontal = 32.dp),
            contentScale = androidx.compose.ui.layout.ContentScale.Fit
        )
        Spacer(modifier = Modifier.height(32.dp))
        Text(
            text = "Sign in to claim and control your devices.",
            color = MaterialTheme.colorScheme.onBackground,
            style = MaterialTheme.typography.bodyLarge
        )
        Spacer(modifier = Modifier.height(24.dp))
        OutlinedTextField(value = email, onValueChange = { email = it }, label = { Text("Email") }, modifier = Modifier.fillMaxWidth())
        Spacer(modifier = Modifier.height(8.dp))
        OutlinedTextField(value = password, onValueChange = { password = it }, label = { Text("Password") }, visualTransformation = PasswordVisualTransformation(), modifier = Modifier.fillMaxWidth())
        Spacer(modifier = Modifier.height(16.dp))
        if (error.isNotEmpty()) {
            Text(error, color = MaterialTheme.colorScheme.error)
            Spacer(modifier = Modifier.height(12.dp))
        }
        if (isLoading) {
            CircularProgressIndicator()
        } else {
            Button(onClick = {
                isLoading = true; error = ""
                coroutineScope.launch {
                    try {
                        auth.signInWithEmailAndPassword(email, password).await()
                        if (!auth.currentUser!!.isEmailVerified) {
                            auth.currentUser!!.sendEmailVerification().await()
                            error = "Verify your email before claiming or controlling a device."
                            isLoading = false
                        } else onLoginSuccess()
                    } catch (exception: Exception) {
                        error = exception.localizedMessage ?: "Sign-in failed"; isLoading = false
                    }
                }
            }, modifier = Modifier.fillMaxWidth()) { Text("Sign In") }
            Spacer(modifier = Modifier.height(8.dp))
            OutlinedButton(onClick = {
                isLoading = true; error = ""
                coroutineScope.launch {
                    try {
                        auth.createUserWithEmailAndPassword(email, password).await()
                        auth.currentUser!!.sendEmailVerification().await()
                        error = "Account created. Verify your email, then sign in."
                    } catch (exception: Exception) {
                        error = exception.localizedMessage ?: "Account creation failed"
                    } finally { isLoading = false }
                }
            }, modifier = Modifier.fillMaxWidth()) { Text("Create Account") }
            Spacer(modifier = Modifier.height(8.dp))
            OutlinedButton(onClick = {
                val webClientIdRes = context.resources.getIdentifier("default_web_client_id", "string", context.packageName)
                if (webClientIdRes == 0) {
                    error = "Google sign-in is not configured for this Firebase project yet. Add a Web client OAuth ID, then download google-services.json again."
                    return@OutlinedButton
                }
                val options = GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN)
                    .requestIdToken(context.getString(webClientIdRes))
                    .requestEmail()
                    .build()
                googleLauncher.launch(GoogleSignIn.getClient(context, options).signInIntent)
            }, modifier = Modifier.fillMaxWidth()) { Text("Continue with Google") }
        }
    }
}
