import telnetlib, sys
import os

ip = sys.argv[1] if len(sys.argv) > 1 else os.getenv("ESP32_IP", "192.168.1.139")
print(f"Connecting to ESP32 at {ip}...")
tn = telnetlib.Telnet(ip, 2323)
tn.interact()
