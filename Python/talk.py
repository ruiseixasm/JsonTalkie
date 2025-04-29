from typing import Dict, Any
import time
import uuid

from broadcast_socket_udp import *
from json_talkie import *


manifesto: Dict[str, Any] = {
    'talk': {
        'name': f"Talker-{str(uuid.uuid4())[:8]}",
        'description': 'A simple Talker!'
    }
}


if __name__ == "__main__":

    broadcast_socket: BroadcastSocket = BroadcastSocket_UDP()
    json_talkie: JsonTalkie = JsonTalkie(broadcast_socket, manifesto)

    # Start listening (opens socket)
    if not json_talkie.on():
        print("Failed to turn jsonTalkie On!")
        exit(1)
    
    print(f"{manifesto['talk']['name']} running. Press Ctrl+C to stop.")
    
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




