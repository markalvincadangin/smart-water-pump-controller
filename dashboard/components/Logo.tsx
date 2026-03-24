import Image from "next/image";

interface LogoProps {
  size?: "sm" | "md" | "lg";
  className?: string;
  variant?: "mark" | "icon";
}

const sizes = {
  sm: 24,
  md: 32,
  lg: 40,
};

export default function Logo({ size = "md", className = "", variant = "mark" }: LogoProps) {
  const s = sizes[size];
  const src = variant === "icon" ? "/logos/brandmark.svg" : "/logos/combinationmark.svg";
  return (
    <Image
      src={src}
      width={s}
      height={s}
      className={`app-icon ${className}`}
      unoptimized
      alt="SmartFlow"
    />
  );
}
