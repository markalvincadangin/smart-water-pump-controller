"use client";

import Image from "next/image";

type AppIconName =
  | "menu-vertical"
  | "settings"
  | "bell"
  | "rotate-cw"
  | "logout";

interface AppIconProps {
  name: AppIconName;
  size?: number;
  className?: string;
  title?: string;
}

const ICON_PATHS: Record<AppIconName, string> = {
  "menu-vertical": "/icons/heroicons/menu-vertical-svgrepo-com.svg",
  settings: "/icons/heroicons/settings-svgrepo-com.svg",
  bell: "/icons/heroicons/bell-svgrepo-com.svg",
  "rotate-cw": "/icons/heroicons/reboot-svgrepo-com.svg",
  logout: "/icons/heroicons/logout-svgrepo-com.svg",
};

export default function AppIcon({ name, size = 16, className, title }: AppIconProps) {
  return (
    <Image
      src={ICON_PATHS[name]}
      width={size}
      height={size}
      className={`app-icon ${className ?? ""}`}
      unoptimized
      alt={title ?? ""}
      aria-hidden={title ? undefined : true}
    />
  );
}
