"use client";

import { useEffect, useMemo, useState } from "react";
import { limitToLast, onValue, query, ref } from "firebase/database";
import { db } from "@/lib/firebase";
import type { AuditEvent } from "@/lib/audit";

export interface AuditEventItem extends AuditEvent {
  id: string;
}

function extractMs(e: AuditEvent): number {
  if (typeof e.at_ms === "number") return e.at_ms;
  if (typeof e.at === "number") return e.at;
  // serverTimestamp() may materialize as an object depending on rules/client; treat as unknown.
  return 0;
}

export function useAuditEvents(limit = 10) {
  const [events, setEvents] = useState<AuditEventItem[]>([]);

  const q = useMemo(() => {
    const base = ref(db, "/pump_system/audit/events");
    return query(base, limitToLast(limit));
  }, [limit]);

  useEffect(() => {
    const unsub = onValue(
      q,
      (snap) => {
        const val = snap.val() as Record<string, AuditEvent> | null;
        if (!val) {
          setEvents([]);
          return;
        }
        const next = Object.entries(val)
          .map(([id, e]) => ({ id, ...e }))
          .sort((a, b) => {
            return extractMs(b) - extractMs(a);
          });
        setEvents(next);
      },
      () => {
        // ignore
      }
    );
    return () => unsub();
  }, [q]);

  return { events };
}

