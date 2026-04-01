"use client";

import React, { useState } from "react";
import { Play, Square, Timer, ShieldAlert, Check, Plus } from "lucide-react";
import clsx from "clsx";

interface ControlPanelProps {
  currentMode: string; // AUTO, MANUAL, COUNTDOWN
  manualDesired: boolean;
  isEmergencyStopLatched: boolean;
  isPending?: boolean;
  countdownRemainingSec?: number;
  onSetMode: (mode: "AUTO" | "MANUAL" | "COUNTDOWN") => void;
  onSetManualDesired: (on: boolean) => void;
  onStartCountdown: (min: number) => void;
  onAddCountdownTime: (min: number) => void;
  onEmergencyStop: () => void;
  onResetStop: () => void;
  startLevel?: number;
  stopLevel?: number;
  isLoading?: boolean;
}

/**
 * REFACTOR [D4.4]: Operator Controls
 * Consolidated panel for system mode selection and direct pump interaction.
 * Includes safety confirmation for emergency actions.
 */
export default function ControlPanel({
  currentMode,
  manualDesired,
  isEmergencyStopLatched,
  isPending = false,
  countdownRemainingSec = 0,
  onSetMode,
  onSetManualDesired,
  onStartCountdown,
  onAddCountdownTime,
  onEmergencyStop,
  onResetStop,
  startLevel,
  stopLevel,
  isLoading = false,
}: ControlPanelProps) {
  const [timerDuration, setTimerDuration] = useState(15);
  const [confirmingEStop, setConfirmingEStop] = useState(false);
  const [confirmingReset, setConfirmingReset] = useState(false);
  const [confirmingManualMode, setConfirmingManualMode] = useState(false);

  if (isLoading) {
    return (
      <div className="card p-6 space-y-6 animate-pulse min-h-[300px]">
        <div className="h-4 w-24 skeleton" />
        <div className="h-10 w-full skeleton rounded-chip" />
        <div className="h-24 w-full skeleton rounded-lg" />
        <div className="h-12 w-full skeleton rounded-chip mt-auto" />
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
      <h3 className="card-header">Controls</h3>

      {/* Mode Selector (Segmented) */}
      <div className="flex p-1 bg-sf-gray-50 rounded-chip border border-[var(--card-border)]/50 gap-1">
        <ModeButton 
          id="AUTO" 
          label="AUTO" 
          active={currentMode === "AUTO"} 
          onClick={() => handleModeChange("AUTO")} 
          disabled={isPending || isEmergencyStopLatched}
        />
        <ModeButton 
          id="MANUAL" 
          label="MANUAL" 
          active={currentMode === "MANUAL"} 
          onClick={() => handleModeChange("MANUAL")} 
          disabled={isPending || isEmergencyStopLatched}
        />
        <ModeButton 
          id="TIMER" 
          label="TIMER" 
          active={currentMode === "COUNTDOWN"} 
          onClick={() => handleModeChange("COUNTDOWN")} 
          disabled={isPending || isEmergencyStopLatched}
        />
      </div>

      {/* Mode-Specific Actions */}
      <div className="flex-1 min-h-[100px] flex flex-col justify-center border-y border-[var(--card-border)]/30 py-4">
        
        {/* MANUAL: Pump Toggle */}
        {currentMode === "MANUAL" && !confirmingManualMode && (
          <div className="flex flex-col gap-3 animate-fade-in">
             <button
                onClick={() => onSetManualDesired(!manualDesired)}
                disabled={isPending}
                className={clsx(
                  "w-full flex items-center justify-center gap-2 py-3 rounded-chip font-bold transition-all transform active:scale-95",
                  manualDesired ? "btn-ghost" : "btn-primary"
                )}
             >
                {manualDesired ? <Square size={18} fill="currentColor" /> : <Play size={18} fill="currentColor" />}
                {manualDesired ? "Stop Pump" : "Start Pump"}
             </button>
             <p className="text-[10px] text-center text-[var(--text-muted)] italic">
               Manual mode bypasses automatic level thresholds.
             </p>
          </div>
        )}

        {/* Manual Confirm Speedbump */}
        {confirmingManualMode && (
           <div className="bg-sf-amber-light/50 border border-sf-amber/20 rounded-lg p-4 animate-slide-up">
              <p className="text-sm font-medium text-sf-amber-dark mb-3">Switch to manual control?</p>
              <div className="flex gap-2">
                 <button onClick={confirmManualMode} className="btn-primary flex-1 py-2 text-sm">Switch</button>
                 <button onClick={() => setConfirmingManualMode(false)} className="btn-ghost flex-1 py-2 text-sm">Cancel</button>
              </div>
           </div>
        )}

        {/* TIMER (Countdown) Controls */}
        {currentMode === "COUNTDOWN" && (
           <div className="flex flex-col gap-4 animate-fade-in">
              {countdownRemainingSec > 0 ? (
                <div className="flex flex-col gap-3">
                   <div className="flex items-center justify-between px-2">
                      <span className="text-xs font-mono font-medium text-sf-blue">Time remaining</span>
                      <span className="font-mono text-xl font-bold text-sf-blue">
                         {Math.floor(countdownRemainingSec / 60)}:{(countdownRemainingSec % 60).toString().padStart(2, '0')}
                      </span>
                   </div>
                   <button 
                      onClick={() => onAddCountdownTime(5)}
                      className="btn-ghost flex items-center justify-center gap-2 py-2 text-sm"
                   >
                      <Plus size={16} />
                      Add 5 minutes
                   </button>
                </div>
              ) : (
                <div className="flex flex-col gap-3">
                    <div className="flex items-center gap-3">
                       <input 
                          type="number" 
                          min="1" 
                          max="120"
                          value={timerDuration}
                          onChange={(e) => setTimerDuration(parseInt(e.target.value) || 1)}
                          className="flex-1 bg-white border border-[var(--card-border)] rounded-chip px-3 py-2 font-mono text-sm focus:outline-sf-blue"
                       />
                       <span className="text-xs font-medium text-[var(--text-muted)]">min</span>
                    </div>
                    <button 
                       onClick={() => onStartCountdown(timerDuration)}
                       disabled={isPending}
                       className="btn-primary py-2.5 flex items-center justify-center gap-2"
                    >
                       <Timer size={18} />
                       Start Timer
                    </button>
                </div>
              )}
           </div>
        )}

        {/* AUTO state info */}
        {currentMode === "AUTO" && (
          <div className="flex flex-col items-center justify-center gap-2 text-[var(--text-muted)] animate-fade-in">
             <div className="h-10 w-10 rounded-full bg-sf-teal-light flex items-center justify-center text-sf-teal mb-1">
                <Check size={20} />
             </div>
             <p className="text-xs font-medium uppercase tracking-wider">System Automatic</p>
             <p className="text-[10px] text-center max-w-[160px] leading-relaxed">
                Pump will start at {startLevel ?? 20}% and stop at {stopLevel ?? 90}%.
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
                  className="btn-danger w-full flex items-center justify-center gap-2 py-3 shadow-lg shadow-sf-red/10 group active:scale-95 transition-transform"
               >
                  <ShieldAlert size={18} className="group-hover:animate-pulse" />
                  EMERGENCY STOP
               </button>
             ) : (
               <div className="bg-sf-red-light border border-sf-red/20 rounded-chip p-3 flex flex-col gap-3 animate-slide-up shadow-xl shadow-sf-red/5">
                  <span className="text-sf-red text-sm font-bold text-center">🛑 STOP PUMP NOW?</span>
                  <div className="flex gap-2">
                     <button onClick={onEmergencyStop} className="btn-danger flex-1 py-1.5 text-xs">CONFIRM</button>
                     <button onClick={() => setConfirmingEStop(false)} className="btn-ghost flex-1 py-1.5 text-xs">CANCEL</button>
                  </div>
               </div>
             )}
           </>
         ) : (
           <>
              {!confirmingReset ? (
                <button 
                   onClick={() => setConfirmingReset(true)}
                   className="btn-primary w-full py-3 animate-pulse"
                >
                   RESET LATCHED STOP
                </button>
              ) : (
                <div className="bg-sf-blue-light border border-sf-blue/20 rounded-chip p-3 flex flex-col gap-3 animate-slide-up">
                   <span className="text-sf-blue text-sm font-bold text-center">RESET SYSTEM?</span>
                   <div className="flex gap-2">
                      <button onClick={onResetStop} className="btn-primary flex-1 py-1.5 text-xs">RESET</button>
                      <button onClick={() => setConfirmingReset(false)} className="btn-ghost flex-1 py-1.5 text-xs">CANCEL</button>
                   </div>
                </div>
              )}
           </>
         )}
      </div>
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
        "flex-1 py-2 text-[10px] font-bold tracking-widest rounded-chip transition-all",
        active 
          ? "bg-sf-blue text-white shadow-sm" 
          : "text-[var(--text-muted)] hover:bg-sf-gray-100 disabled:opacity-50"
      )}
    >
      {label}
    </button>
  );
}
