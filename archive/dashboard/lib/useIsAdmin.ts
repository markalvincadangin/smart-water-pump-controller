"use client";

import { useEffect, useState } from "react";
import { ref, get } from "firebase/database";
import { db } from "@/lib/firebase";

export function useIsAdmin(uid: string | null) {
  const [isAdmin, setIsAdmin] = useState(false);

  useEffect(() => {
    if (!uid) {
      setIsAdmin(false);
      return;
    }

    const adminsRef = ref(db, `/pump_system/config/admins/${uid}`);

    get(adminsRef)
      .then((snap) => {
        setIsAdmin(snap.exists() === true && snap.val() === true);
      })
      .catch(() => {
        setIsAdmin(false);
      });
  }, [uid]);

  return isAdmin;
}

