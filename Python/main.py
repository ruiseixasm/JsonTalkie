from typing import Dict, Any
import time

from broadcast_socket_udp import *
from json_talkie import *


import numpy as np
import simpleaudio as sa

def buzz(duration_ms=500, freq=1000, volume=0.5):
    """High-quality buzzer sound"""
    sample_rate = 44100  # CD quality
    t = np.linspace(0, duration_ms/1000, int(sample_rate * duration_ms/1000), False)
    wave = np.sin(2 * np.pi * freq * t) * volume
    
    # Convert to 16-bit PCM
    audio = (wave * 32767).astype(np.int16)
    play_obj = sa.play_buffer(audio, 1, 2, sample_rate)
    play_obj.wait_done()

manifesto: Dict[str, Any] = {
    'talk': {
        'name': 'Buzzer',
        'description': 'This device does a 500ms buzz!'
    },
    'list': {
        'call': {
            'buzz': {
                'description': 'Triggers a 500ms buzzing sound',
                'function': buzz
            }
        }
    }
}



talker_name: str = manifesto['talk']['name']
last_message: Dict[str, Any] = {}
message_time: float = 0.0

def wait(seconds: float = 2) -> bool:
    return last_message and time.time() - message_time < seconds
    

def receive(message: Dict[str, Any]) -> bool:
    """Handles message content only."""
    match message['talk']:
        case "talk":
            print(f"[{manifesto['talk']['name']}] \t {manifesto['talk']['description']}")
        case "call":
            function = manifesto['list']['call'][message['function']]['function']
            function()
        case "echo":
            if message['id'] == last_message['id']:
                match last_message['talk']:
                    case "list":
                        print(f"[{message['from']}] Listed")
                    case "call":
                        print(f"[{message['from']}] Executed")
                        last_message = {}
        case _:
            print("Unknown talking!")
    return False

if __name__ == "__main__":

    broadcast_socket: BroadcastSocket = BroadcastSocket_UDP()
    json_talkie: JsonTalkie = JsonTalkie(broadcast_socket)

    # Start listening (opens socket)
    if not json_talkie.on(receive):
        print("Failed to turn jsonTalkie On!")
        exit(1)
    
    print(f"Talker {talker_name} running. Press Ctrl+C to stop.")
    
    try:
        message: Dict[str, Any] = {
            'talk': 'call', 'function': 'buzz',
            'to': 'Buzzer', 'from': "Dummy"
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


