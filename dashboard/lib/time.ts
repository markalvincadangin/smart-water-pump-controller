const PHT_TIMEZONE = "Asia/Manila";
const PHT_LOCALE = "en-PH";

export function formatPhtTime(value: number | Date): string {
  const date = value instanceof Date ? value : new Date(value);
  return date.toLocaleTimeString(PHT_LOCALE, {
    hour12: false,
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    timeZone: PHT_TIMEZONE,
  });
}

export function formatPhtDateTime(value: number | Date): string {
  const date = value instanceof Date ? value : new Date(value);
  return date.toLocaleString(PHT_LOCALE, {
    year: "numeric",
    month: "short",
    day: "2-digit",
    hour12: false,
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    timeZone: PHT_TIMEZONE,
  });
}

export function getPhtTimezoneLabel(): string {
  return "PHT (UTC+8)";
}
