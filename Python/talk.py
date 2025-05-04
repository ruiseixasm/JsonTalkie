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
from typing import Dict, Any, List
from prompt_toolkit import PromptSession    # python3.13 -m pip install prompt_toolkit
from prompt_toolkit.history import FileHistory
import os
import time
import uuid

from broadcast_socket_udp import *
from broadcast_socket_dummy import *
from broadcast_socket_serial import *
from json_talkie import *

class CommandLine:
    def __init__(self):
        # Ensure history file exists
        if not os.path.exists(".cmd_history"):
            open(".cmd_history", 'w').close()
        # Defines 'talk', 'run', 'set', 'get' parameters
        self.manifesto: Dict[str, Dict[str, Any]] = {
            'talker': {
                'name': f"Talker-{str(uuid.uuid4())[:2]}",
                'description': 'A simple Talker!'
            },
            'echo': self.echo
        }

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
            if words:
                message: Dict[str, Any] = {
                    "c": words[0]
                }
                match words[0]:
                    case "talk":
                        if len(words) == 2: # Targeted talk
                            message["t"] = words[1]
                        if len(words) < 3:
                            json_talkie.talk(message)
                            time.sleep(0.5) # Wait some time
                        else:
                            print(f"\t'{words[0]}' has a wrong number of arguments!")
                    case "list":
                        if len(words) == 2: # Targeted talk
                            message["t"] = words[1]
                            json_talkie.talk(message)
                            time.sleep(0.5) # Wait some time
                        else:
                            print(f"\t'{words[0]}' has a wrong number of arguments!")
                    case "run" | "get":
                        if len(words) == 3:
                            message["t"] = words[1]
                            message["w"] = words[2]
                            json_talkie.talk(message)
                            time.sleep(0.5) # Wait some time
                        else:
                            print(f"\t'{words[0]}' has a wrong number of arguments!")
                    case "set":
                        if len(words) == 4:
                            try:
                                message["t"] = words[1]
                                message["w"] = words[2]
                                message["v"] = int(words[3])
                                json_talkie.talk(message)
                                time.sleep(0.5) # Wait some time
                            except Exception as e:
                                print(f"\t'{words[3]}' is not an integer!")
                        else:
                            print(f"'{words[0]}' has a wrong number of arguments!")
                    case "sys":
                        if len(words) == 2: # Targeted talk
                            message["t"] = words[1]
                        if len(words) < 3:
                            json_talkie.talk(message)
                            time.sleep(0.5) # Wait some time
                        else:
                            print(f"\t'{words[0]}' has a wrong number of arguments!")
                    case _:
                        print(f"\t[talk]\tPrints all devices' 'name' and description.")
                        print(f"\t[list 'device']\tList the entire 'device' manifesto.")
                        print(f"\t[run 'device' 'what']\tRuns the named function.")
                        print(f"\t[set 'device' 'what']\tSets the named variable.")
                        print(f"\t[get 'device' 'what']\tGets the named variable value.")
                        print(f"\t[sys]\tPrints the platform of the Device.")
                        print(f"\t[exit]\tExits the command line (Ctrl+D).")
                        print(f"\t[help]\tShows the present help.")                        

    def echo(self, message: Dict[str, Any]) -> bool:
        if "w" in message:
            if "v" in message:
                print(f"\t[{message["f"]} {message["w"]}]\t{message["r"]}\t{message["v"]}")
            elif "l" in message:
                print(f"\t[{message["l"]} {message["f"]} {message["w"]}]\t{message["r"]}")
            else:
                print(f"\t[{message["f"]} {message["w"]}]\t{message["r"]}")
        else:
            print(f"\t[{message["f"]}]\t{message["r"]}")
        return True


if __name__ == "__main__":

    SOCKET = "SERIAL"

    broadcast_socket: BroadcastSocket = None
    match SOCKET:
        case "SERIAL":
            broadcast_socket = BroadcastSocket_Serial("COM5")
        case "DUMMY":
            broadcast_socket = BroadcastSocket_Dummy()
        case _:
            broadcast_socket = BroadcastSocket_UDP()

    cli = CommandLine()
    json_talkie: JsonTalkie = JsonTalkie(broadcast_socket, cli.manifesto)

    # Start listening (opens socket)
    if not json_talkie.on():
        print("\tFailed to turn jsonTalkie On!")
        exit(1)
    
    print(f"\t[{cli.manifesto['talker']['name']}] running. Type 'exit' to exit or 'talk' to make them talk.")
    cli.run()

    json_talkie.off()  # Turns jsonTalkie Off




