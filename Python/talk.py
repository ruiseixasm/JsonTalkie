from typing import Dict, Any
import time
import uuid

from broadcast_socket_udp import *
from json_talkie import *
from walkie_device import *


def receive(self, message: Dict[str, Any]) -> bool:
    """Handles message content only."""
    if message['from'] != self._name: # Makes sure sender doesn't process it's own messages
        match message['talk']:
            case "echo":
                if message['id'] == self._last_message['id']:
                    match self._last_message['talk']:
                        case "list":
                            print(f"[{message['from']}] Listed")
                        case "call":
                            print(f"[{message['from']}] Executed")
                            self._last_message = {}
            case _:
                self.roger(message)
    return False


if __name__ == "__main__":

    broadcast_socket: BroadcastSocket = BroadcastSocket_UDP()
    json_talkie: JsonTalkie = JsonTalkie(broadcast_socket)
    talker_name: str = f"Talker-{str(uuid.uuid4())[:8]}"
    last_message: Dict[str, Any] = {}
    message_time: float = 0.0

    # Start listening (opens socket)
    if not json_talkie.on(receive):
        print("Failed to turn jsonTalkie On!")
        exit(1)
    
    print(f"{talker_name} running. Press Ctrl+C to stop.")
    
    print("Welcome to My Command Line!")
    print("Type 'help' to see available commands.")
    
    while True:
        command: str = input("> ").strip()
        
        if command == "help":
            print("Available commands: greet, bye, exit")
        
        elif command == "talk":
            
            try:
                json_talkie.talk(
                    {'talk': 'call', 'function': 'buzz', 'to': 'Buzzer'}
                )
                time.sleep(2)  # Send ping every 2 seconds
            
            except KeyboardInterrupt:
                print("\nShutting down...")


        elif command == "exit":
            print("Exiting...")
            break
        
        elif len(command) > 0:
            try:
                json_talkie.talk(
                    { 'talk': command }
                )
                time.sleep(2)  # Send ping every 2 seconds
            
            except KeyboardInterrupt:
                print("\nShutting down...")
        
    json_talkie.off()  # Turns jsonTalkie Off




