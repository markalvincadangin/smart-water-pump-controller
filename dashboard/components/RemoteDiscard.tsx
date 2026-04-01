import React from 'react';

interface RemoteDiscardProps {
  discardCount?: number;
  showTooltip?: boolean;
}

/**
 * RemoteDiscard: Displays LDSC (Level Discard Count) from the sensor node.
 * 
 * Phase 5: Shows the counter of discarded ultrasonic readings that failed validation.
 * Useful for diagnosing sensor noise or environmental interference.
 * 
 * Low values (<5) = healthy sensor.
 * High values (>50) = potential sensor degradation or noise issues.
 */
export function RemoteDiscard({
  discardCount = 0,
  showTooltip = true,
}: RemoteDiscardProps) {
  const isWarning = (discardCount ?? 0) >= 20 && (discardCount ?? 0) < 50;
  const isAlarm = (discardCount ?? 0) >= 50;

  const bgColor = isAlarm
    ? 'bg-red-100 dark:bg-red-900/20 border-red-300 dark:border-red-700'
    : isWarning
      ? 'bg-yellow-100 dark:bg-yellow-900/20 border-yellow-300 dark:border-yellow-700'
      : 'bg-gray-100 dark:bg-gray-800/50 border-gray-300 dark:border-gray-700';

  const textColor = isAlarm
    ? 'text-red-700 dark:text-red-300'
    : isWarning
      ? 'text-yellow-700 dark:text-yellow-300'
      : 'text-gray-700 dark:text-gray-300';

  return (
    <div
      className={`inline-flex items-center gap-1.5 px-2 py-1 text-xs rounded border ${bgColor} ${textColor}`}
      title={
        showTooltip
          ? `Level Discard Count: ${discardCount} discarded sensor readings`
          : undefined
      }
    >
      <svg
        className="w-3.5 h-3.5"
        fill="currentColor"
        viewBox="0 0 20 20"
      >
        <path
          fillRule="evenodd"
          d="M18 5v8a2 2 0 01-2 2h-5l-5 4v-4H4a2 2 0 01-2-2V5a2 2 0 012-2h12a2 2 0 012 2zm-11-1a1 1 0 11-2 0 1 1 0 012 0zM8 7a1 1 0 000 2h6a1 1 0 000-2H8z"
          clipRule="evenodd"
        />
      </svg>
      <span className="font-medium">LDSC: {discardCount}</span>
    </div>
  );
}

export default RemoteDiscard;
