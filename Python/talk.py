from typing import Dict, Any, List
from prompt_toolkit import PromptSession
from prompt_toolkit.history import FileHistory
import os
import time
import uuid

from broadcast_socket_udp import *
from json_talkie import *

class CommandLine:
    def __init__(self):
        # Ensure history file exists
        if not os.path.exists(".cmd_history"):
            open(".cmd_history", 'w').close()
            
        self.session = PromptSession(history=FileHistory('.cmd_history'))

    def run(self):
        print("Interactive Command Line (Ctrl+D to exit)")
        while True:
            try:
                cmd = self.session.prompt(">>> ").strip()
                if not cmd:
                    continue
                
                self._execute(cmd)
                
            except EOFError:  # Ctrl+D
                print("\nExiting...")
                break
            except KeyboardInterrupt:  # Ctrl+C
                print("\nUse Ctrl+D to exit")
                continue
            except Exception as e:
                print(f"Error: {e}")

    def _execute(self, cmd: str):
        """Handle command execution"""
        cmd = cmd.strip().lower()
        if cmd in ("exit", "quit"):
            raise EOFError
        elif cmd == "history":
            # Print history from file
            with open('.cmd_history', 'r') as f:
                for i, line in enumerate(f, 1):
                    print(f"{i}: {line.strip()}")
        else:
            words = cmd.split()
            if words[0] in ("call", "list", "run", "set", "get"):
                message: Dict[str, Any] = {
                    'type': words[0]
                }
                match words[0]:
                    case "call":
                        json_talkie.talk(message)
                        time.sleep(1)  # Send ping every 2 seconds
                    case "list":
                        if len(words) > 1:
                            message['to'] = words[1]
                            json_talkie.talk(message)
                            time.sleep(1)  # Send ping every 2 seconds
                        else:
                            print(f"{words[0]} has not enough arguments!")
                    case "run":
                        if len(words) > 2:
                            message['to'] = words[1]
                            message['function'] = words[2]
                            json_talkie.talk(message)
                        else:
                            print(f"{words[0]} has not enough arguments!")
            else:
                print(f"{words[0]} is not a valid command type!")



manifesto: Dict[str, Dict[str, Any]] = {
    'talker': {
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
    
    print(f"{manifesto['talker']['name']} running. Press Ctrl+C to stop.")
    print("Welcome to My Command Line!")
    print("Type 'help' to see available commands.")

    cli = CommandLine()
    cli.run()

    json_talkie.off()  # Turns jsonTalkie Off




