from typing import Dict, Any
import time

from broadcast_socket_udp import *
from json_talkie import *


class Talker:
    def __init__(self):
        # Defines 'talk', 'run', 'set', 'get' parameters
        self.manifesto: Dict[str, Dict[str, Any]] = {
            'talker': {
                'name': 'Buzzer',
                'description': 'This device does a 500ms buzz!'
            },
            'run': {
                'buzz': {
                    'description': 'Triggers a 500ms buzzing sound',
                    'function': self.buzz
                },
                'print':{
                    'description': 'Prints the duration on the device',
                    'function': self.print_duration
                }
            },
            'set': {
                'duration': {
                    'description': 'Sets the duration of Buzzing in seconds',
                    'function': self.set_duration
                }
            },
            'get': {
                'duration': {
                    'description': 'Gets the duration of Buzzing in seconds',
                    'function': self.get_duration
                }
            },
            'echo': self.echo
        }
        # Talker self variables
        self._duration: float = 0.5

    def buzz(self):
        print(f"\tBUZZING for {self._duration} seconds!\a")

    def print_duration(self):
        print(f"\t{self._duration}")

    def set_duration(self, duration: float) -> bool:
        self._duration = duration
        return True

    def get_duration(self) -> float:
        return self._duration
    
    def echo(self, message: Dict[str, Any], response: str) -> bool:
        print(f"\t{response}")
        return True


if __name__ == "__main__":

    talker = Talker()
    broadcast_socket: BroadcastSocket = BroadcastSocket_UDP()
    json_talkie: JsonTalkie = JsonTalkie(broadcast_socket, talker.manifesto)

    # Start listening (opens socket)
    if not json_talkie.on():
        print("\tFailed to turn jsonTalkie On!")
        exit(1)
    
    print(f"\tTalker {talker.manifesto['talker']['name']} running. Press Ctrl+C to stop.")
    
    try:
        message: Dict[str, Any] = {
            'type': 'run', 'what': 'buzz', 'to': 'Buzzer'
        }
        last_message = message
        # Main loop
        while True:
            message_time = time.time()
            json_talkie.talk(message)
            time.sleep(30)  # Send ping every 3 seconds
    except KeyboardInterrupt:
        print("\tShutting down...")
    finally:
        json_talkie.off()  # Ensures socket cleanup


