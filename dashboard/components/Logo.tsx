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
  const isIcon = variant === "icon";
  return (
    <svg
      width={s}
      height={s}
      viewBox="0 0 32 32"
      fill="none"
      xmlns="http://www.w3.org/2000/svg"
      className={`text-accent-cyan ${className}`}
      aria-hidden
    >
      {isIcon && (
        <rect width="32" height="32" rx="8" fill="#0A0E1A" />
      )}
      <path
        d="M16 4c0 0-8 6-8 12 0 4.4 3.6 8 8 8s8-3.6 8-8c0-6-8-12-8-12z"
        fill="currentColor"
      />
    </svg>
  );
}
