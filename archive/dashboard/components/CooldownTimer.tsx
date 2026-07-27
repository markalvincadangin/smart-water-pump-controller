import React, { useEffect, useState } from 'react';

interface CooldownTimerProps {
  remainingSeconds?: number;
  runMode?: string;
}

/**
 * CooldownTimer: Displays pump cooldown countdown when active (AUTO_COOLDOWN or MANUAL_COOLDOWN).
 * 
 * Phase 5: Shows time remaining until the pump can be reactivated after a cooldown cycle.
 * Updates every 100ms for smooth display.
 */
export function CooldownTimer({
  remainingSeconds = 0,
  runMode,
}: CooldownTimerProps) {
  const [displaySeconds, setDisplaySeconds] = useState(remainingSeconds);

  useEffect(() => {
    setDisplaySeconds(remainingSeconds);
  }, [remainingSeconds]);

  // Only show cooldown if actively in one of the cooldown modes
  const isInCooldown =
    runMode === 'AUTO_COOLDOWN' || runMode === 'MANUAL_COOLDOWN';

  if (!isInCooldown || displaySeconds <= 0) {
    return null;
  }

  const minutes = Math.floor(displaySeconds / 60);
  const seconds = displaySeconds % 60;

  return (
    <div className="inline-flex items-center gap-2 px-3 py-1.5 rounded-full bg-yellow-100 dark:bg-yellow-900/30 border border-yellow-300 dark:border-yellow-700">
      <svg
        className="w-4 h-4 text-yellow-600 dark:text-yellow-400 animate-spin"
        fill="none"
        viewBox="0 0 24 24"
      >
        <circle
          className="opacity-25"
          cx="12"
          cy="12"
          r="10"
          stroke="currentColor"
          strokeWidth="4"
        />
        <path
          className="opacity-75"
          fill="currentColor"
          d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"
        />
      </svg>
      <span className="text-sm font-medium text-yellow-700 dark:text-yellow-300">
        {minutes > 0 ? `${minutes}m ${seconds}s` : `${seconds}s`} cooldown
      </span>
    </div>
  );
}

export default CooldownTimer;
