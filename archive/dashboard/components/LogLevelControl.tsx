import React, { useState } from 'react';

interface LogLevelControlProps {
  currentLevel?: number;
  isAdmin?: boolean;
  onLevelChange?: (level: number) => Promise<void>;
}

const LOG_LEVELS = [
  { value: 0, label: 'ERROR', color: 'text-red-600 dark:text-red-400' },
  { value: 1, label: 'WARN', color: 'text-yellow-600 dark:text-yellow-400' },
  { value: 2, label: 'INFO', color: 'text-blue-600 dark:text-blue-400' },
  { value: 3, label: 'DEBUG', color: 'text-green-600 dark:text-green-400' },
  { value: 4, label: 'VERBOSE', color: 'text-gray-600 dark:text-gray-400' },
];

/**
 * LogLevelControl: Shows current debug log level and allows admins to adjust it.
 * 
 * Phase 5: Displays the current firmware logging level (0–4) and provides a dropdown
 * for admins to adjust remote log filtering. Changes are pushed to Firebase.
 */
export function LogLevelControl({
  currentLevel = 2,
  isAdmin = false,
  onLevelChange,
}: LogLevelControlProps) {
  const [isChanging, setIsChanging] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const currentLabel =
    LOG_LEVELS.find((l) => l.value === currentLevel)?.label || 'UNKNOWN';
  const currentColor =
    LOG_LEVELS.find((l) => l.value === currentLevel)?.color || '';

  const handleChange = async (newLevel: number) => {
    if (!isAdmin || !onLevelChange) return;
    setIsChanging(true);
    setError(null);
    try {
      await onLevelChange(newLevel);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to update log level');
    } finally {
      setIsChanging(false);
    }
  };

  if (!isAdmin) {
    // Read-only display for non-admins
    return (
      <div className="inline-flex items-center gap-2">
        <span className="text-xs font-medium text-gray-500 dark:text-gray-400">
          Log Level:
        </span>
        <span className={`text-sm font-semibold ${currentColor}`}>
          [{currentLabel}]
        </span>
      </div>
    );
  }

  return (
    <div className="inline-flex items-center gap-3">
      <span className="text-xs font-medium text-gray-500 dark:text-gray-400">
        Log Level:
      </span>
      <select
        value={currentLevel}
        onChange={(e) => handleChange(parseInt(e.target.value))}
        disabled={isChanging}
        className={`px-2 py-1 text-sm font-medium rounded border ${
          isChanging
            ? 'opacity-50 cursor-not-allowed'
            : 'border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800'
        }`}
      >
        {LOG_LEVELS.map((level) => (
          <option key={level.value} value={level.value}>
            {level.label}
          </option>
        ))}
      </select>
      {error && (
        <span className="text-xs text-red-600 dark:text-red-400">{error}</span>
      )}
    </div>
  );
}

export default LogLevelControl;
