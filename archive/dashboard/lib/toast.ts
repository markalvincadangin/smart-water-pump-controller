"use client";

export type ToastKind = "success" | "info" | "warning" | "error";

export interface ToastMessage {
  id?: string;
  kind?: ToastKind;
  title: string;
  detail?: string;
  timeoutMs?: number;
}

const EVENT_NAME = "dashboard:toast";

export function toast(message: ToastMessage) {
  if (typeof window === "undefined") return;
  const evt = new CustomEvent<ToastMessage>(EVENT_NAME, { detail: message });
  window.dispatchEvent(evt);
}

export function onToast(listener: (msg: ToastMessage) => void) {
  if (typeof window === "undefined") return () => {};

  const handler = (e: Event) => {
    const ce = e as CustomEvent<ToastMessage>;
    if (!ce.detail?.title) return;
    listener(ce.detail);
  };

  window.addEventListener(EVENT_NAME, handler);
  return () => window.removeEventListener(EVENT_NAME, handler);
}

