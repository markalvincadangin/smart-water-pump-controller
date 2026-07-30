# Design Tokens

This document defines the implementation contract between design and code.

## Token source

The platform-neutral source is:

`tokens/smartflow.tokens.json`

Platform exports are derived from that file.

## Naming structure

`category.role.variant.state`

Examples:

- `color.status.critical.action`
- `space.2`
- `radius.md`
- `motion.duration.standard`
- `type.title.medium`

Components must consume semantic tokens rather than raw palette values.

## Color tokens

### Brand

```text
brand.blue       #0EA5E9
brand.green      #10B981
brand.navy       #0F172A
brand.white      #FFFFFF
```

### Semantic

```text
status.criticalAccent  #EF4444
status.criticalAction  #DC2626
status.warning         #F59E0B
status.advisory        #0EA5E9
status.success         #10B981
```

### Foreground corrections

```text
onPrimary    #0F172A
onSecondary  #0F172A
onTertiary   #451A03
onError      #FFFFFF
```

## Spacing tokens

```text
space.0    0dp
space.0_5  4dp
space.1    8dp
space.1_5  12dp
space.2    16dp
space.3    24dp
space.4    32dp
space.6    48dp
space.8    64dp
```

## Shape tokens

```text
radius.xs    4dp
radius.sm    8dp
radius.md   16dp
radius.lg   24dp
radius.full 999dp
```

## Motion tokens

```text
instant     0ms
fast      100ms
standard  200ms
emphasized 300ms
slow      500ms
```

## Android implementation

The package includes reference files:

- `implementation/android/Color.kt`
- `implementation/android/Theme.kt`
- `implementation/android/Type.kt`
- `implementation/android/Shape.kt`

These are implementation references and should be integrated into the app package namespace and resource setup.

## Governance

- Token changes require design and engineering review.
- Semantic meaning must not change silently.
- Deprecate tokens before removal.
- Record token changes in the changelog.
- Do not use raw hex values inside feature components.
