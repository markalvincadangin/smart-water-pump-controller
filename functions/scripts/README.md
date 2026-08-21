# Firebase Admin Scripts

## Reset Test Environment

### Purpose
Resets Firebase Authentication and Realtime Database for fresh testing of the provisioning flow.

### Safety Features

- Defaults to a read-only plan; no implicit project or database target.
- Requires explicit `--project`, `--database-url`, `--apply`, and a destructive acknowledgement.
- Refuses project IDs named like production projects.
- Writes an ignored, local RTDB backup before making changes.
- Preserves Firebase project settings, deployed Functions, OAuth providers, and local secrets.

### Usage

1. **Preview the exact reset:**
   ```bash
   cd functions
   npm run reset-test-env -- --project your-test-project --database-url https://your-test-project-default-rtdb.asia-southeast1.firebasedatabase.app
   ```

2. **Apply only after reviewing the plan.** For a clean ESP32 provisioning test, reseed the one required non-secret bootstrap registry entry after the root wipe:
   ```bash
   npm run reset-test-env -- --project your-test-project --database-url https://your-test-project-default-rtdb.asia-southeast1.firebasedatabase.app --apply --acknowledge-delete-all-test-data --seed-device-id SF-TEST01 --seed-secret-name projects/your-test-project/secrets/smartflow-bootstrap-sf-test01
   ```

The command uses Application Default Credentials. Authenticate your shell first, for example with `firebase login` / an existing Firebase CLI session or an explicitly configured service account.

### What Gets Deleted

**Authentication:**
- All user accounts (Google, Email/Password, Anonymous, etc.)

**Realtime Database:**

- The complete RTDB root, including device data, ownership mappings, pairing material, audit events, telemetry, and user indexes.

The pre-reset RTDB snapshot is written beneath `functions/.local/reset-backups/` and is ignored by Git. Auth accounts cannot be exported by this utility; record any required test identities before applying.

### Bootstrap registry seed

`--seed-device-id` and `--seed-secret-name` are optional but must be supplied together. The seed is written only after the full RTDB root and all Auth users have been deleted. It recreates just:

`deviceRegistry/{deviceId} = { state: \"active\", secretName, updatedAtMs, updatedBy }`

`secretName` is a Secret Manager resource name—not the secret value. The option never restores device metadata, ownership, pairing proofs, telemetry, audit history, user indexes, Functions, OAuth providers, Secret Manager values, or ignored local firmware configuration.

### Scope

This is intentionally a full test-environment reset. Do not alter it into a production or partial-delete tool. Use purpose-built, dry-run-first per-device scripts for migration and ownership maintenance.

### Alternative: Firebase Console

If you prefer a visual approach:

**Delete Users:**
1. [Firebase Console](https://console.firebase.google.com) → Your Project
2. Authentication → Users
3. Select and delete users

**Clear Database:**
1. Realtime Database → Data tab
2. Click root node → ⋮ menu → Delete

### Alternative: Firebase CLI

For one-off operations:

```bash
# Delete a specific database path
firebase database:remove /devices

# Delete all auth users (requires confirmation)
firebase auth:export users.json
# (Review the export, then manually delete via console)
```

## Other Scripts

### Migrate Legacy Ownership

Dry-run-first. It migrates only consistent legacy ownership records and freezes ambiguous records; it never guesses or transfers an owner.

```bash
npm run migrate-legacy-ownership -- --project your-test-project --database-url https://your-test-project-default-rtdb.asia-southeast1.firebasedatabase.app
```

Apply a reviewed plan:

```bash
npm run migrate-legacy-ownership -- --project your-test-project --database-url https://your-test-project-default-rtdb.asia-southeast1.firebasedatabase.app --apply --acknowledge-ownership-migration
```

### Resolve Ownership Conflicts

Operator-only and per-device. The operator must have Firebase's `admin` custom claim, the selected owner must be a durable account, and the supplied non-secret evidence reference becomes part of the audit record.

```bash
npm run resolve-ownership-conflict -- --project your-test-project --database-url https://your-test-project-default-rtdb.asia-southeast1.firebasedatabase.app --device-id SF-TEST01 --owner-uid <durable-owner-uid> --operator-uid <admin-uid> --evidence CASE-123
```

Apply a reviewed resolution:

```bash
npm run resolve-ownership-conflict -- --project your-test-project --database-url https://your-test-project-default-rtdb.asia-southeast1.firebasedatabase.app --device-id SF-TEST01 --owner-uid <durable-owner-uid> --operator-uid <admin-uid> --evidence CASE-123 --apply --acknowledge-ownership-resolution
```
