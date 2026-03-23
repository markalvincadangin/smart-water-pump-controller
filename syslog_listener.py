import socket

UDP_IP = "0.0.0.0"
UDP_PORT = 5514

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening for Syslog on UDP port {UDP_PORT}...")
print("Waiting for Sensor Node or Main Controller to connect to Wi-Fi...\n")

while True:
    try:
        data, addr = sock.recvfrom(2048)
        message = data.decode('utf-8', errors='replace').strip()
        print(f"[{addr[0]}] {message}")
    except KeyboardInterrupt:
        print("\nExiting...")
        break
