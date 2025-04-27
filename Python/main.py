from typing import Dict, Any
import time

from broadcast_socket_udp import *
from device import *

if __name__ == "__main__":

    broadcast_socket = BroadcastSocket()
    json_device = JsonDevice(broadcast_socket)

    # Start listening (opens socket)
    if not json_device.start():
        print("Failed to start device!")
        exit(1)
    
    print(f"Device {json_device._name} running. Press Ctrl+C to stop.")
    
    try:
        # Main loop
        while True:
            json_device.send_json({'type': 'ping', 'from': json_device._name})
            time.sleep(3)  # Send ping every 3 seconds
            
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        json_device.stop()  # Ensures socket cleanup


