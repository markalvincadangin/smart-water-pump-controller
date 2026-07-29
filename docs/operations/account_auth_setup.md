# Android Account Authentication Setup

Use this checklist before validating account-backed provisioning or ownership.

1. In the SmartFlow Firebase project, enable **Google** and **Email/Password** sign-in providers.
2. Register the Android app package and its debug/release signing certificate fingerprints in the Firebase project, then replace the local `app/google-services.json` with the matching project file. Do not commit a different project’s configuration.
3. Configure the Google OAuth consent screen and Android OAuth client for the same package and signing fingerprints.
4. Install a debug build on a physical Android device and verify:
   - Google sign-in returns to SmartFlow with a non-anonymous Firebase user.
   - New email/password users receive a verification email.
   - Unverified password users cannot claim or control a device.
   - The same durable account can sign in again and see its existing device list.

The firmware never receives user passwords or Google tokens. It uses its separate per-device bootstrap credential.
