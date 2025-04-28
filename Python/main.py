from typing import Dict, Any
import time

from broadcast_socket_udp import *
from json_talkie import *
from walkie_device import *

if __name__ == "__main__":

    broadcast_socket: BroadcastSocket = BroadcastSocket_UDP()
    json_talkie: JsonTalkie = JsonTalkie(broadcast_socket)
    walkie_device: WalkieDevice = WalkieDevice(json_talkie)

    # Start listening (opens socket)
    if not walkie_device.start():
        print("Failed to start device!")
        exit(1)
    
    print(f"Device {walkie_device._name} running. Press Ctrl+C to stop.")
    
    try:
        # Main loop
        while True:
            walkie_device.talk(
                {'command': 'call', 'function': 'buzz', 'to': 'Buzzer'}
            )
            time.sleep(3)  # Send ping every 3 seconds
            
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        walkie_device.stop()  # Ensures socket cleanup


