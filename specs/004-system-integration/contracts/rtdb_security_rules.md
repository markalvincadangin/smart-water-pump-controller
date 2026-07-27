# Contract: Firebase RTDB Security Rules (V2)

**Feature**: `004-system-integration`
**File**: `database.rules.json`

See [data-model.md](../data-model.md) for the full V2 schema and rule structure.

## Access Control Matrix

| Actor | Path | Read | Write |
|-------|------|------|-------|
| Device Owner (App) | `/users/{ownUid}/` | ✅ | ✅ |
| Device Owner (App) | `/devices/{id}/shadow/desired/` | ✅ | ✅ |
| Device Owner (App) | `/devices/{id}/` (all other sub-paths) | ✅ | ❌ |
| Firmware (Email Auth) | `/devices/{id}/status/`, `telemetry/`, `reported/`, `diagnostics/`, `events/`, `metadata/` | ✅ | ✅ |
| Firmware (Email Auth) | `/devices/{id}/shadow/desired/` | ✅ | ❌ |
| Unauthenticated | Any | ❌ | ❌ |
| Other Auth User | `/devices/{id}/` they don't own | ❌ | ❌ |
