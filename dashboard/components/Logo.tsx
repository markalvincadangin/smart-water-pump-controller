"use client";

interface LogoProps {
  size?: "sm" | "md" | "lg";
  className?: string;
}

const sizes = {
  sm: 24,
  md: 32,
  lg: 40,
};

export default function Logo({ size = "md", className = "" }: LogoProps) {
  const s = sizes[size];
  return (
    <svg
      width={s}
      height={Math.round(s * 1.2)}
      viewBox="0 0 20 24"
      fill="none"
      xmlns="http://www.w3.org/2000/svg"
      className={`text-accent-cyan ${className}`}
      aria-hidden
    >
      <path
        d="M10 0C10 0 4 8 4 14C4 18.4183 6.58172 22 10 22C13.4183 22 16 18.4183 16 14C16 8 10 0 10 0Z"
        fill="currentColor"
      />
    </svg>
  );
}
