'''
JsonTalkie - Json Talkie is intended for direct IoT communication.
Original Copyright (c) 2025 Rui Seixas Monteiro. All right reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
https://github.com/ruiseixasm/JsonTalkie
'''
from typing import Dict, Any
import time

from broadcast_socket_udp import *
from broadcast_socket_dummy import *
from broadcast_socket_serial import *
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

    def buzz(self) -> str:
        print(f"\tBUZZING for {self._duration} seconds!\a")
        time.sleep(self._duration) # Take its time
        return f"Buzzing done for {self._duration}"

    def print_duration(self) -> str:
        print(f"\t{self._duration}")

    def set_duration(self, duration: str) -> str:
        try:
            self._duration = float(duration)
            return f"Duration is now set to {self._duration}"
        except (ValueError, TypeError):
            # Handle cases where conversion fails
            return f"Duration of '{duration}' is NOT a float!"

    def get_duration(self) -> str:
        return str(self._duration)
    
    def echo(self, message: Dict[str, Any], reply: str) -> bool:
        print(f"\t{reply}")
        return True


if __name__ == "__main__":

    talker = Talker()
    broadcast_socket: BroadcastSocket = BroadcastSocket_Serial()
    json_talkie: JsonTalkie = JsonTalkie(broadcast_socket, talker.manifesto)

    # Start listening (opens socket)
    if not json_talkie.on():
        print("\tFailed to turn jsonTalkie On!")
        exit(1)
    
    print(f"\tTalker {talker.manifesto['talker']['name']} running. Press Ctrl+C to stop.")
    
    try:
        message: Dict[str, Any] = {
            "c": 'run', 'what': 'buzz', "t": 'Buzzer'
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


