import { ref, set, update } from 'firebase/database';
import { db } from './firebase';
import type { ControlMode, LogLevel } from './types';

/**
 * Typed, error-handled Firebase write actions for pump control.
 * All actions return Promise<void> and throw on error.
 * Callers must wrap in try/catch and set pending state.
 */

export const setMode = (mode: ControlMode) =>
  set(ref(db, '/pump_system/control/mode'), mode);

export const setManualDesired = (desired: boolean) =>
  set(ref(db, '/pump_system/control/manual_desired'), desired);

export const triggerEmergencyStop = () =>
  set(ref(db, '/pump_system/control/emergency_stop'), true);

export const resetEmergencyStop = () =>
  set(ref(db, '/pump_system/control/reset_stop'), true);

export const clearError = () =>
  set(ref(db, '/pump_system/control/clear_error'), true);

export const startCountdown = (durationMin: number) =>
  update(ref(db, '/pump_system/control'), {
    countdown_start: true,
    countdown_duration_min: durationMin,
  });

export const addCountdownTime = (addMin: number) =>
  update(ref(db, '/pump_system/control'), {
    countdown_add_time: true,
    countdown_add_min: addMin,
  });

export const setBypassLevel = (bypass: boolean) =>
  set(ref(db, '/pump_system/control/bypass_level_sensor'), bypass);

export const setBypassFlow = (bypass: boolean) =>
  set(ref(db, '/pump_system/control/bypass_flow_sensor'), bypass);

export const setRemoteLogLevel = (level: LogLevel) =>
  set(ref(db, '/pump_system/config/device/debug_log_level'), level);

export const requestReboot = (currentId: number) =>
  set(ref(db, '/pump_system/control/reboot_request_id'), currentId + 1);
