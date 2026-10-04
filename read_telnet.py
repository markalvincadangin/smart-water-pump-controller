#!/usr/bin/env python3
"""
SmartFlow ESP32 Telnet Monitor
Streams real-time console and safety logs from the ESP32 over TCP (port 2323).
Compatible with Python 3.8+ (does not rely on deprecated telnetlib).
"""

import sys
import os
import re
import socket
import select
import signal
import time

DEFAULT_PORT = 2323
DEFAULT_TIMEOUT_SEC = 5.0
ENV_FILE = ".env"

# ANSI Color Codes
COLOR_RESET = "\033[0m"
COLOR_BOLD = "\033[1m"
COLOR_DIM = "\033[2m"
COLOR_RED = "\033[31m"
COLOR_GREEN = "\033[32m"
COLOR_YELLOW = "\033[33m"
COLOR_BLUE = "\033[34m"
COLOR_MAGENTA = "\033[35m"
COLOR_CYAN = "\033[36m"
COLOR_WHITE = "\033[37m"
COLOR_BRIGHT_RED = "\033[91m"
COLOR_BRIGHT_YELLOW = "\033[93m"
COLOR_BRIGHT_CYAN = "\033[96m"

# Check if terminal supports color (Windows 10+ supports ANSI natively)
SUPPORTS_COLOR = True
if os.name == 'nt':
    try:
        import ctypes
        kernel32 = ctypes.windll.kernel32
        # Enable ENABLE_VIRTUAL_TERMINAL_PROCESSING (0x0004)
        kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
    except Exception:
        pass


def colorize(text: str, color: str) -> str:
    if not SUPPORTS_COLOR:
        return text
    return f"{color}{text}{COLOR_RESET}"


def parse_env_for_ip(env_path: str = ENV_FILE) -> str:
    """Extract ESP32_IP from .env file if available."""
    if not os.path.exists(env_path):
        # Look in workspace root if run from subfolder
        parent_env = os.path.join(os.path.dirname(__file__), ENV_FILE)
        if os.path.exists(parent_env):
            env_path = parent_env
        else:
            return ""

    try:
        with open(env_path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line.startswith("#") or "=" not in line:
                    continue
                k, v = line.split("=", 1)
                if k.strip() == "ESP32_IP":
                    return v.strip().strip('"').strip("'")
    except Exception:
        pass
    return ""


def update_env_ip(new_ip: str, env_path: str = ENV_FILE) -> bool:
    """Save new ESP32_IP back to .env for future convenience."""
    target_path = env_path
    if not os.path.exists(target_path):
        target_path = os.path.join(os.path.dirname(__file__), ENV_FILE)

    if not os.path.exists(target_path):
        return False

    try:
        with open(target_path, "r", encoding="utf-8") as f:
            lines = f.readlines()

        updated = False
        new_lines = []
        for line in lines:
            if re.match(r"^\s*ESP32_IP\s*=", line):
                new_lines.append(f"ESP32_IP={new_ip}\n")
                updated = True
            else:
                new_lines.append(line)

        if not updated:
            new_lines.append(f"ESP32_IP={new_ip}\n")

        with open(target_path, "w", encoding="utf-8") as f:
            f.writelines(new_lines)
        return True
    except Exception:
        return False


def format_log_line(line: str) -> str:
    """Apply contextual syntax coloring based on log severity."""
    if not SUPPORTS_COLOR:
        return line

    if "--- Buffered Logs ---" in line:
        return colorize(f"\n{line}", COLOR_BOLD + COLOR_MAGENTA)
    if "--- Live Logs ---" in line:
        return colorize(f"\n{line}\n", COLOR_BOLD + COLOR_CYAN)

    # Highlight log levels
    if "[ERROR]" in line or "E (" in line or "ERROR" in line:
        return colorize(line, COLOR_BRIGHT_RED)
    if "[WARN]" in line or "[WARNING]" in line or "W (" in line:
        return colorize(line, COLOR_BRIGHT_YELLOW)
    if "[INFO]" in line or "I (" in line:
        # Highlight tags like [BOOT], [SAFETY], [STATE], [PUMP]
        return re.sub(
            r"(\[INFO\]|I\s+\([^)]+\))\s+(\[[A-Z0-9_-]+\])?",
            lambda m: f"{colorize(m.group(1), COLOR_GREEN)} {colorize(m.group(2) or '', COLOR_BOLD + COLOR_CYAN)}",
            line
        )
    if "[DEBUG]" in line or "D (" in line:
        return colorize(line, COLOR_DIM)

    return line


def connect_and_stream(ip: str, port: int, auto_reconnect: bool = True):
    """Establish socket connection and stream logs with clean reconnection."""
    attempt = 1
    max_retries = 3 if not auto_reconnect else 999999

    while attempt <= max_retries:
        print(colorize(f"[*] Connecting to SmartFlow ESP32 Telnet console at {ip}:{port} (Attempt {attempt})...", COLOR_CYAN))
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(DEFAULT_TIMEOUT_SEC)

        try:
            start_t = time.time()
            sock.connect((ip, port))
            sock.settimeout(None)  # Set blocking for select/read
            latency_ms = int((time.time() - start_t) * 1000)
            print(colorize(f"[+] Connected successfully! Latency: {latency_ms}ms.", COLOR_BOLD + COLOR_GREEN))
            print(colorize("[*] Streaming live ESP32 logs. Press Ctrl+C to exit.\n", COLOR_DIM))
            attempt = 1  # Reset retry counter on successful connection

            buffer = ""
            while True:
                data = sock.recv(4096)
                if not data:
                    print(colorize("\n[!] Connection closed by ESP32 (remote host disconnected).", COLOR_YELLOW))
                    break

                # Decode UTF-8 lossy for robust handling of serial characters
                text = data.decode("utf-8", errors="replace")
                buffer += text

                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    line = line.rstrip("\r")
                    if line:
                        print(format_log_line(line), flush=True)

        except (socket.timeout, TimeoutError):
            print(colorize(f"\n[-] Connection timed out ({DEFAULT_TIMEOUT_SEC}s) reaching {ip}:{port}.", COLOR_BRIGHT_RED))
            print_troubleshooting(ip, port)
        except ConnectionRefusedError:
            print(colorize(f"\n[-] Connection refused by {ip}:{port}.", COLOR_BRIGHT_RED))
            print_troubleshooting(ip, port, refused=True)
        except OSError as e:
            print(colorize(f"\n[-] Network error: {e}", COLOR_BRIGHT_RED))
            print_troubleshooting(ip, port)
        except KeyboardInterrupt:
            print(colorize("\n\n[*] Monitor disconnected by user.", COLOR_YELLOW))
            try:
                sock.close()
            except Exception:
                pass
            return
        finally:
            try:
                sock.close()
            except Exception:
                pass

        if not auto_reconnect:
            break

        print(colorize("[*] Reconnecting in 3 seconds... (Press Ctrl+C to stop)", COLOR_DIM))
        try:
            time.sleep(3)
        except KeyboardInterrupt:
            print(colorize("\n[*] Monitor stopped.", COLOR_YELLOW))
            return
        attempt += 1


def print_troubleshooting(ip: str, port: int, refused: bool = False):
    """Print helpful diagnostic hints when connection fails."""
    print(colorize("\n--- Troubleshooting Checklist ---", COLOR_BOLD + COLOR_YELLOW))
    print(f" 1. Is the ESP32 powered on?")
    print(f" 2. Has the ESP32 connected to Wi-Fi? (Check router DHCP leases)")
    if refused:
        print(f" 3. The IP responded, but port {port} is closed. Ensure TelnetSink is initialized in firmware.")
    else:
        print(f" 3. Verify IP address '{ip}' is correct. If changed, pass the new IP:")
        print(f"    pwsh -File .vscode/run_task.ps1 firmware_monitor <NEW_IP>")
        print(f"    or update .env (ESP32_IP=<NEW_IP>)")
    print(colorize("---------------------------------\n", COLOR_BOLD + COLOR_YELLOW))


def main():
    # Setup clean Ctrl+C handler
    def sig_handler(sig, frame):
        print(colorize("\n[*] Exiting...", COLOR_YELLOW))
        sys.exit(0)
    signal.signal(signal.SIGINT, sig_handler)

    # Determine IP and Port
    ip = ""
    port = DEFAULT_PORT

    # 1. Check CLI arguments
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if len(args) >= 1:
        ip = args[0]
    if len(args) >= 2:
        try:
            port = int(args[1])
        except ValueError:
            print(colorize(f"Invalid port: {args[1]}, using default {DEFAULT_PORT}", COLOR_YELLOW))

    # 2. Check Environment Variable
    if not ip:
        ip = os.environ.get("ESP32_IP", "").strip()

    # 3. Check .env file
    if not ip:
        ip = parse_env_for_ip()

    # 4. Interactive prompt if still missing
    if not ip:
        print(colorize("[?] ESP32 IP address not specified in arguments, environment, or .env.", COLOR_YELLOW))
        try:
            entered_ip = input("Enter ESP32 IP address (e.g., 192.168.1.13): ").strip()
            if entered_ip:
                ip = entered_ip
                save = input("Save this IP to .env for future use? (y/N): ").strip().lower()
                if save == 'y':
                    if update_env_ip(ip):
                        print(colorize(f"[+] Saved ESP32_IP={ip} to .env", COLOR_GREEN))
        except (KeyboardInterrupt, EOFError):
            print("\nAborted.")
            sys.exit(1)

    if not ip:
        print(colorize("[-] Error: No IP address provided. Exiting.", COLOR_RED))
        sys.exit(1)

    connect_and_stream(ip, port, auto_reconnect=False)


if __name__ == "__main__":
    main()
