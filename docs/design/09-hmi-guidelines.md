# Safety-Critical HMI Guidelines

SmartFlow controls physical equipment. The interface is ISA-101-inspired but must not claim certification unless formally assessed.

## 1. Calm normal state

Normal operation is visually quiet.

- muted surfaces
- blue for normal telemetry
- green only for confirmed healthy states
- no celebratory animation
- no persistent glow

The absence of red and amber should make normality obvious.

## 2. Alarm lifecycle

An alarm has two independent dimensions:

- **Condition:** active or cleared
- **Acknowledgement:** acknowledged or unacknowledged

Supported states:

1. Active / Unacknowledged
2. Active / Acknowledged
3. Cleared / Unacknowledged
4. Normal

Acknowledgement means the operator has seen the alarm. It does not mean the condition is resolved.

## 3. Alarm priorities

### Priority 1 — Critical

- Color: critical red
- Immediate equipment or property risk
- System may automatically stop
- Requires immediate operator attention

Examples: dry run, overflow, emergency stop

### Priority 2 — Warning

- Color: amber
- Degraded condition that may become critical

Examples: stale sensor data, low tank level, retrying connection

### Priority 3 — Advisory

- Color: blue
- Informational event with no immediate hazard

Examples: scheduled start, maintenance reminder, firmware available

## 4. Actionable alarm content

Every alarm must state:

1. What happened
2. What the system did
3. What the operator should do
4. Whether remote control is available
5. When the alarm began

Example:

**Dry run detected**  
Pump stopped to prevent damage. Restore water supply, then acknowledge and reset.

## 5. Command authority and interlocks

The UI must show:

- current mode
- who or what has control
- active interlocks
- command destination
- whether remote control is permitted

A disabled command must explain why.

## 6. Command feedback

When a command is sent:

1. Show `Pending`
2. Prevent duplicate submission
3. Wait for hardware or trusted gateway acknowledgement
4. Show `Completed`, `Rejected`, or `Timed out`
5. Keep an audit timestamp

Never infer success solely because the cloud request was accepted.

## 7. Data freshness

Every critical value must have a timestamp or freshness status.

- live
- delayed
- stale
- unavailable

Stale data must never look live. Freeze the last known value and label it clearly.

## 8. Situational awareness

Show trends, thresholds, and operating context.

- current value
- recent direction
- safe range
- alarm threshold
- missing-data gaps

Do not smooth charts in a way that hides spikes or gaps.

## 9. Human error prevention

Destructive actions require deliberate confirmation.

Recommended pattern:

`Open confirmation → Review outcome → Hold or clearly confirm`

Avoid long multi-screen flows during emergencies.

## 10. Dashboard hierarchy

1. Global critical alarm / connectivity state
2. Pump state and control authority
3. Tank level and interlocks
4. Flow and trend context
5. Historical and advanced data

## 11. Auditability

Record:

- command
- user or automation source
- target device
- timestamp
- result
- rejection reason
- alarm acknowledgement
- alarm clearance

The UI should expose recent activity without overwhelming the main dashboard.
