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
        while True:
            try:
                cmd = self.session.prompt(">>> ").strip()
                if not cmd:
                    continue
                
                self._execute(cmd)
                
            except EOFError:  # Ctrl+D
                print("\tExiting...")
                break
            except KeyboardInterrupt:  # Ctrl+C
                print("\tUse Ctrl+D to exit")
                continue
            except Exception as e:
                print(f"\tError: {e}")

    def _execute(self, cmd: str):
        """Handle command execution"""
        cmd = cmd.strip()
        if cmd in ("exit", "quit"):
            raise EOFError
        elif cmd == "history":
            # Print history from file
            with open('.cmd_history', 'r') as f:
                for i, line in enumerate(f, 1):
                    print(f"{i}: {line.strip()}")
        else:
            words = cmd.split()
            if words[0] in ("talk", "run", "set", "get"):
                message: Dict[str, Any] = {
                    'type': words[0]
                }
                match words[0]:
                    case "talk":
                        if len(words) == 2: # Targeted talk
                            message["to"] = words[1]
                        if len(words) < 3:
                            json_talkie.talk(message)
                            time.sleep(0.5) # Wait some time
                        else:
                            print(f"'{words[0]}' has a wrong number of arguments!")
                    case "run" | "get":
                        if len(words) == 3:
                            message['to'] = words[1]
                            message['what'] = words[2]
                            json_talkie.talk(message)
                            time.sleep(0.5) # Wait some time
                        else:
                            print(f"'{words[0]}' has a wrong number of arguments!")
                    case "set":
                        if len(words) == 4:
                            message['to'] = words[1]
                            message['what'] = words[2]
                            message['value'] = words[3]
                            json_talkie.talk(message)
                            time.sleep(0.5) # Wait some time
                        else:
                            print(f"'{words[0]}' has a wrong number of arguments!")
            else:
                print(f"\t'{words[0]}' is not a valid command type!")



manifesto: Dict[str, Dict[str, Any]] = {
    'talker': {
        'name': f"Talker-{str(uuid.uuid4())[:2]}",
        'description': 'A simple Talker!'
    }
}


if __name__ == "__main__":

    broadcast_socket: BroadcastSocket = BroadcastSocket_UDP()
    json_talkie: JsonTalkie = JsonTalkie(broadcast_socket, manifesto)

    # Start listening (opens socket)
    if not json_talkie.on():
        print("\tFailed to turn jsonTalkie On!")
        exit(1)
    
    print(f"\t[{manifesto['talker']['name']}] running. Type 'exit' to exit or 'talk' to make them talk.")
    cli = CommandLine()
    cli.run()

    json_talkie.off()  # Turns jsonTalkie Off




