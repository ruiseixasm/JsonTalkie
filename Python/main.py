from typing import Dict, Any
import time

from broadcast_socket_udp import *
from json_talkie import *


def buzz():
    print("BUZZING")
    print('\a')

# Defines 'talk', 'call', 'list', 'run', 'set', 'get' parameters
manifesto: Dict[str, Dict[str, Any]] = {
    'talker': {
        'name': 'Buzzer',
        'description': 'This device does a 500ms buzz!'
    },
    'run': {
        'buzz': {
            'description': 'Triggers a 500ms buzzing sound',
            'function': buzz
        }
    }
}


if __name__ == "__main__":

    broadcast_socket: BroadcastSocket = BroadcastSocket_UDP()
    json_talkie: JsonTalkie = JsonTalkie(broadcast_socket, manifesto)

    # Start listening (opens socket)
    if not json_talkie.on():
        print("Failed to turn jsonTalkie On!")
        exit(1)
    
    print(f"Talker {manifesto['talker']['name']} running. Press Ctrl+C to stop.")
    
    try:
        message: Dict[str, Any] = {
            'type': 'run', 'function': 'buzz', 'to': 'Buzzer'
        }
        last_message = message
        # Main loop
        while True:
            message_time = time.time()
            json_talkie.talk(message)
            time.sleep(3)  # Send ping every 3 seconds
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        json_talkie.off()  # Ensures socket cleanup


