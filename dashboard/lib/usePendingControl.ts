"use client";

import { useEffect, useState } from "react";
import type { PumpControl } from "@/lib/types";
import { toast } from "@/lib/toast";

export function usePendingControl(currentMode: PumpControl["mode"]) {
  const [pendingMode, setPendingMode] = useState<PumpControl["mode"] | null>(null);
  const [pendingAck, setPendingAck] = useState(false);

  useEffect(() => {
    if (pendingMode && currentMode === pendingMode) {
      setPendingMode(null);
      toast({ kind: "success", title: `Mode confirmed: ${currentMode}` });
    }
  }, [currentMode, pendingMode]);

  return {
    pendingMode,
    setPendingMode,
    pendingAck,
    setPendingAck,
  };
}

