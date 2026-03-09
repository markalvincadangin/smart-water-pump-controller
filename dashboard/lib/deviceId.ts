"use client";

const KEY = "dashboard_device_id";

function newId() {
  return `dev_${Math.random().toString(16).slice(2)}_${Date.now().toString(16)}`;
}

export function getDashboardDeviceId(): string {
  if (typeof window === "undefined") return "server";
  try {
    const existing = localStorage.getItem(KEY);
    if (existing) return existing;
    const created = newId();
    localStorage.setItem(KEY, created);
    return created;
  } catch {
    return `mem_${newId()}`;
  }
}

