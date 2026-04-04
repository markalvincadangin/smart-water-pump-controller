"use client";

import React, { useState, useEffect } from "react";
import { Play, Square, Timer, ShieldAlert, Check, Plus } from "lucide-react";
import clsx from "clsx";
import type { ControlMode } from "@/lib/types";

interface ControlPanelProps {
  currentMode: ControlMode; // AUTO, MANUAL, COUNTDOWN
  manualDesired: boolean;
  isEmergencyStopLatched: boolean;
  safetyHold?: boolean;
  isPending?: boolean;
  countdownRemainingSec?: number;
  onSetMode: (mode: "AUTO" | "MANUAL" | "COUNTDOWN") => void;
  onSetManualDesired: (on: boolean) => void;
  onStartCountdown: (min: number) => void;
  onAddCountdownTime: (min: number) => void;
  onStopCountdown?: () => void;
  onEmergencyStop: () => void;
  onResetStop: () => void;
  startLevel?: number;
  stopLevel?: number;
  isLoading?: boolean;
  /** Actual hardware run_mode from status — used to show cooldown feedback. */
  pumpRunMode?: string;
}

/**
 * Consolidated panel for system mode selection and direct pump interaction.
 * Includes safety confirmation for emergency actions.
 */
export default function ControlPanel({
  currentMode,
  manualDesired,
  isEmergencyStopLatched,
  safetyHold = false,
  isPending = false,
  countdownRemainingSec = 0,
  onSetMode,
  onSetManualDesired,
  onStartCountdown,
  onAddCountdownTime,
  onStopCountdown,
  onEmergencyStop,
  onResetStop,
  startLevel,
  stopLevel,
  isLoading = false,
  pumpRunMode = "",
}: ControlPanelProps) {
  const [timerDuration, setTimerDuration] = useState(15);
  const [confirmingEStop, setConfirmingEStop] = useState(false);
  const [confirmingReset, setConfirmingReset] = useState(false);
  const [confirmingManualMode, setConfirmingManualMode] = useState(false);

  // Smooth local timer interpolation to prevent jumping every 3s from Firebase
  const [localSec, setLocalSec] = useState(countdownRemainingSec);

  useEffect(() => {
    // If we drift by more than 3 seconds or the timer just started/stopped, snap to server truth
    if (Math.abs(localSec - countdownRemainingSec) > 3) {
      setLocalSec(countdownRemainingSec);
    }
  }, [countdownRemainingSec, localSec]);

  useEffect(() => {
    if (currentMode !== "COUNTDOWN" || localSec <= 0) return;
    const interval = setInterval(() => {
      setLocalSec((prev) => Math.max(0, prev - 1));
    }, 1000);
    return () => clearInterval(interval);
  }, [currentMode, localSec]);

  if (isLoading) {
    return (
      <div className="card p-6 space-y-6 animate-pulse min-h-[300px]">
        <div className="h-4 w-24 skeleton" />
        <div className="h-10 w-full skeleton rounded-md" />
        <div className="h-24 w-full skeleton" />
        <div className="h-12 w-full skeleton rounded-md mt-auto" />
      </div>
    );
  }

  const handleModeChange = (newMode: "AUTO" | "MANUAL" | "COUNTDOWN") => {
    if (newMode === currentMode) return;
    
    // Safety: Special confirm for switching TO manual mode
    if (newMode === "MANUAL") {
      setConfirmingManualMode(true);
      return;
    }
    
    onSetMode(newMode);
  };

  const confirmManualMode = () => {
    onSetMode("MANUAL");
    setConfirmingManualMode(false);
  };

  return (
    <div className="card p-6 flex flex-col gap-6 relative">
      <h3 className="text-sm font-semibold uppercase tracking-wider text-[var(--text-muted)] self-start">Control Panel</h3>

      {/* Mode Selector (Segmented) */}
      <div className="flex p-1 bg-sf-gray-100 dark:bg-sf-gray-900 rounded-lg border border-[var(--card-border)]/50 gap-1">
        <ModeButton 
          id="AUTO" 
          label="Auto" 
          active={currentMode === "AUTO"} 
          onClick={() => handleModeChange("AUTO")} 
          disabled={isPending || isEmergencyStopLatched || safetyHold}
        />
        <ModeButton 
          id="MANUAL" 
          label="Manual" 
          active={currentMode === "MANUAL"} 
          onClick={() => handleModeChange("MANUAL")} 
          disabled={isPending || isEmergencyStopLatched || safetyHold}
        />
        <ModeButton 
          id="TIMER" 
          label="Timer" 
          active={currentMode === "COUNTDOWN"} 
          onClick={() => handleModeChange("COUNTDOWN")} 
          disabled={isPending || isEmergencyStopLatched || safetyHold}
        />
      </div>

      {/* Mode-Specific Actions */}
      <div className="flex-1 min-h-[100px] flex flex-col justify-center border-y border-[var(--card-border)]/50 py-5">
        
        {/* MANUAL: Pump Toggle */}
        {currentMode === "MANUAL" && !confirmingManualMode && (
          <div className="flex flex-col gap-3 animate-fade-in">
             <button
                onClick={() => onSetManualDesired(!manualDesired)}
                disabled={isPending || safetyHold}
                aria-label={manualDesired ? "Stop pump (manual mode)" : "Start pump (manual mode)"}
                className={clsx(
                  "w-full flex items-center justify-center gap-2 py-3 rounded-md font-medium transition-all transform active:scale-[0.98] border shadow-sm",
                  manualDesired 
                    ? "bg-[var(--card-bg)] text-[var(--text-primary)] border-[var(--card-border)] hover:bg-sf-gray-50 dark:hover:bg-sf-gray-900" 
                    : "bg-[var(--text-primary)] text-[var(--page-bg)] border-transparent hover:opacity-90"
                )}
             >
                {manualDesired ? <Square size={16} fill="currentColor" /> : <Play size={16} fill="currentColor" />}
                {manualDesired ? "Stop Pump" : "Start Pump"}
             </button>
             <p className="text-[11px] text-center text-[var(--text-muted)] mt-1">
               Manual mode bypasses automatic level thresholds.
             </p>
             {manualDesired && pumpRunMode === "MANUAL_COOLDOWN" && (
               <div className="flex items-center justify-center gap-2 text-[11px] font-mono text-sf-blue animate-pulse mt-1">
                 <div className="w-1.5 h-1.5 rounded-full bg-sf-blue" />
                 Cooldown active — pump will start shortly
               </div>
             )}
          </div>
        )}

        {/* Manual Confirm Speedbump */}
        {confirmingManualMode && (
           <div className="bg-sf-amber/10 border border-sf-amber/20 rounded-md p-4 animate-slide-up">
              <p className="text-sm font-medium text-sf-amber-dark dark:text-sf-amber mb-4">Switch to manual control?</p>
              <div className="flex gap-2">
                 <button onClick={confirmManualMode} className="bg-sf-amber text-white px-4 py-2 rounded-md text-sm font-medium shadow-sm hover:bg-sf-amber-dark transition-colors flex-1">Switch</button>
                 <button onClick={() => setConfirmingManualMode(false)} className="bg-[var(--card-bg)] border border-[var(--card-border)] px-4 py-2 rounded-md text-sm font-medium hover:bg-sf-gray-50 transition-colors flex-1">Cancel</button>
              </div>
           </div>
        )}

        {/* TIMER (Countdown) Controls */}
        {currentMode === "COUNTDOWN" && (
           <div className="flex flex-col gap-4 animate-fade-in">
              {localSec > 0 ? (
                <div className="flex flex-col gap-4">
                   <div className="flex items-center justify-between px-1">
                      <span className="text-xs font-medium text-[var(--text-secondary)]">Time remaining</span>
                      <span className="font-mono text-2xl font-semibold tabular-nums tracking-tight">
                         {Math.floor(localSec / 60)}:{(localSec % 60).toString().padStart(2, '0')}
                      </span>
                   </div>
                   <div className="flex gap-2">
                       <button 
                          onClick={() => onAddCountdownTime(5)}
                          disabled={isPending || safetyHold}
                          className="flex-1 flex items-center justify-center gap-2 py-2.5 rounded-md border border-[var(--card-border)] hover:bg-sf-gray-50 dark:hover:bg-sf-gray-900 text-sm font-medium transition-colors"
                          aria-label="Add 5 minutes to countdown"
                       >
                          <Plus size={16} />
                          Add 5 min
                       </button>
                       <button 
                          onClick={() => onStopCountdown?.()}
                          disabled={isPending || safetyHold}
                          className="flex-1 flex items-center justify-center gap-2 py-2.5 rounded-md border border-sf-amber/30 bg-sf-amber/10 text-sf-amber-dark dark:text-sf-amber hover:bg-sf-amber/20 text-sm font-medium transition-colors"
                          aria-label="Stop countdown timer"
                       >
                          <Square size={16} />
                          Stop Timer
                       </button>
                   </div>
                </div>
              ) : (
                <div className="flex flex-col gap-4">
                    <div className="flex items-center gap-3">
                       <input 
                          type="number" 
                          min="1" 
                          max="120"
                          value={timerDuration}
                          onChange={(e) => setTimerDuration(parseInt(e.target.value) || 1)}
                          aria-label="Countdown duration minutes"
                          className="flex-1 bg-[var(--card-bg)] border border-[var(--card-border)] rounded-md px-3 py-2 font-mono text-sm focus:outline-none focus:ring-2 focus:ring-sf-blue/50"
                       />
                       <span className="text-xs font-medium text-[var(--text-muted)]">min</span>
                    </div>
                    <button 
                       onClick={() => onStartCountdown(timerDuration)}
                       disabled={isPending || safetyHold}
                       className="bg-[var(--text-primary)] text-[var(--page-bg)] py-2.5 rounded-md flex items-center justify-center gap-2 font-medium hover:opacity-90 transition-opacity active:scale-[0.98]"
                       aria-label="Start countdown timer"
                    >
                       <Timer size={16} />
                       Start Timer
                    </button>
                </div>
              )}
           </div>
        )}

        {/* AUTO state info */}
        {currentMode === "AUTO" && (
          <div className="flex flex-col items-center justify-center gap-3 text-[var(--text-muted)] animate-fade-in py-2">
             <div className="h-8 w-8 rounded-full bg-sf-green/10 flex items-center justify-center text-sf-green">
                <Check size={16} />
             </div>
             <p className="text-sm font-medium text-[var(--text-secondary)]">System Automatic</p>
             <p className="text-xs text-center max-w-[180px] leading-relaxed">
                Pump starts at {startLevel ?? 20}% and stops at {stopLevel ?? 90}%.
             </p>
          </div>
        )}
      </div>

      {/* Emergency Stop at the bottom */}
      <div className="mt-2">
         {!isEmergencyStopLatched ? (
           <>
             {!confirmingEStop ? (
               <button 
                  onClick={() => setConfirmingEStop(true)}
                className="hidden md:flex w-full items-center justify-center gap-2 py-3 border border-sf-red/30 bg-sf-red/5 text-sf-red font-semibold rounded-md hover:bg-sf-red hover:text-white transition-all transform active:scale-[0.98]"
                  aria-label="Emergency stop — open confirmation"
                disabled={isPending}
               >
                  <ShieldAlert size={16} />
                  Emergency Stop
               </button>
             ) : (
               <div className="bg-sf-red/10 border border-sf-red/30 rounded-md p-4 flex flex-col gap-4 animate-slide-up shadow-sm">
                  <span className="text-sf-red text-sm font-semibold text-center">Stop pump immediately?</span>
                  <div className="flex gap-2">
                     <button
                       onClick={() => {
                         onEmergencyStop();
                         setConfirmingEStop(false);
                       }}
                       className="bg-sf-red text-white flex-1 py-2 rounded-md font-medium text-sm hover:bg-sf-red-dark transition-colors"
                       aria-label="Confirm emergency stop"
                       disabled={isPending}
                     >
                       Confirm
                     </button>
                     <button
                       onClick={() => setConfirmingEStop(false)}
                       className="border border-sf-red/30 text-sf-red hover:bg-sf-red/5 flex-1 py-2 rounded-md font-medium text-sm transition-colors"
                       aria-label="Cancel emergency stop confirmation"
                       disabled={isPending}
                     >
                       Cancel
                     </button>
                  </div>
               </div>
             )}
           </>
         ) : (
           <>
              {!confirmingReset ? (
                <button 
                   onClick={() => setConfirmingReset(true)}
                   className="w-full flex items-center justify-center gap-2 py-3 bg-[var(--text-primary)] text-[var(--page-bg)] font-semibold rounded-md animate-pulse"
                   aria-label="Reset latched emergency stop — open confirmation"
                   disabled={isPending}
                >
                   Reset Latched Stop
                </button>
              ) : (
                <div className="bg-sf-blue/10 border border-sf-blue/30 rounded-md p-4 flex flex-col gap-4 animate-slide-up">
                   <span className="text-sf-blue text-sm font-semibold text-center">Reset system to Auto?</span>
                   <div className="flex gap-2">
                      <button
                        onClick={() => {
                          onResetStop();
                          setConfirmingReset(false);
                        }}
                        className="bg-sf-blue text-white flex-1 py-2 rounded-md font-medium text-sm hover:opacity-90 transition-opacity"
                        aria-label="Confirm reset emergency stop latch"
                        disabled={isPending}
                      >
                        Reset
                      </button>
                      <button
                        onClick={() => setConfirmingReset(false)}
                        className="border border-sf-blue/30 text-sf-blue hover:bg-sf-blue/5 flex-1 py-2 rounded-md font-medium text-sm transition-colors"
                        aria-label="Cancel reset confirmation"
                        disabled={isPending}
                      >
                        Cancel
                      </button>
                   </div>
                </div>
              )}
           </>
         )}
      </div>

      {/* Mobile-only sticky E-stop bar (always reachable) */}
      {!isEmergencyStopLatched && !confirmingEStop && (
        <div className="fixed bottom-0 left-0 right-0 p-3 bg-[var(--card-bg)] border-t border-[var(--card-border)] md:hidden z-40">
          <button
            className="w-full flex items-center justify-center gap-2 py-3 bg-sf-red text-white font-semibold rounded-md hover:bg-sf-red-dark active:scale-[0.98] transition-all"
            aria-label="Emergency stop — stops pump immediately"
            onClick={() => setConfirmingEStop(true)}
            disabled={isPending}
          >
            <ShieldAlert size={16} /> Emergency Stop
          </button>
        </div>
      )}
    </div>
  );
}

interface ModeButtonProps {
  id: string;
  label: string;
  active: boolean;
  onClick: () => void;
  disabled: boolean;
}

function ModeButton({ label, active, onClick, disabled }: ModeButtonProps) {
  return (
    <button
      onClick={onClick}
      disabled={disabled}
      className={clsx(
        "flex-1 py-1.5 text-xs font-semibold rounded-md transition-all",
        active 
          ? "bg-[var(--card-bg)] text-[var(--text-primary)] shadow-sm border border-[var(--card-border)]/50" 
          : "text-[var(--text-secondary)] hover:text-[var(--text-primary)] disabled:opacity-50 border border-transparent"
      )}
    >
      {label}
    </button>
  );
}
