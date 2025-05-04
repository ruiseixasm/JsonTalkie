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
import json
import threading
import uuid
from typing import Dict, Any, TYPE_CHECKING, Callable
import time
import platform

from broadcast_socket import BroadcastSocket

# Keys:
#     m: message
#     c: command
#     t: to
#     f: from
#     i: id
#     r: reply
#     w: what
#     s: checksum
#     v: value
#     l: list


class JsonTalkie:

    def __init__(self, socket: BroadcastSocket, manifesto: Dict[str, Dict[str, Any]]):
        self._socket: BroadcastSocket = socket  # Composition over inheritance
        self._manifesto: Dict[str, Dict[str, Any]] = manifesto
        self._last_message: Dict[str, Any] = {}
        self._message_time: float = 0.0
        self._running: bool = False

    def on(self) -> bool:
        """Start message processing (no network knowledge)."""
        if not self._socket.open():
            return False
        self._running = True
        self._thread = threading.Thread(target=self.listen, daemon=True)    # Where the listen is set
        self._thread.start()
        return True
    
    def off(self):
        """Stop processing (delegates cleanup to socket)."""
        self._running = False
        self._receiver = None
        if hasattr(self, '_thread'):
            self._thread.join()
        self._socket.close()

    def talk(self, message: Dict[str, Any]) -> bool:
        """Sends messages without network awareness."""
        message["f"] = self._manifesto['talker']['name']
        if "i" not in message:
            message["i"] = JsonTalkie.message_id()
        if message["c"] != "echo":
            self._last_message = message
        talk: Dict[str, Any] = {
            "m": message,
            "s": JsonTalkie.checksum(message)
        }
        print(f"\tLocal: {talk}")
        return self._socket.send( JsonTalkie.encode(talk) )
    
    def listen(self):
        """Processes raw bytes from socket."""
        while self._running:
            received = self._socket.receive()
            if received:
                data, _ = received  # Explicitly ignore (ip, port)
                print(f"\tReceived data: {data}")
                try:
                    talk: Dict[str, Any] = JsonTalkie.decode(data)
                    if self.validate_talk(talk):
                        print(f"\tValid message!")
                        self.receive(talk["m"])
                except (UnicodeDecodeError, json.JSONDecodeError) as e:
                    print(f"\tInvalid message: {e}")

    def receive(self, message: Dict[str, Any]) -> bool:
        """Handles message content only."""
        match message["c"]:
            case "talk":
                echo: Dict[str, Any] = {
                    "c": 'echo',
                    "t": message["f"],
                    "i": message["i"]
                }
                echo["r"] = f"{self._manifesto['talker']['description']}"
                self.talk(echo)
            case "list":
                echo: Dict[str, Any] = {
                    "c": 'echo',
                    "t": message["f"],
                    "i": message["i"]
                }
                if 'run' in self._manifesto:
                    for key, value in self._manifesto['run'].items():
                        echo["l"] = "run"
                        echo["w"] = key
                        echo["r"] = value['description']
                        self.talk(echo)
                if 'set' in self._manifesto:
                    for key, value in self._manifesto['set'].items():
                        echo["l"] = "set"
                        echo["w"] = key
                        echo["r"] = value['description']
                        self.talk(echo)
                if 'get' in self._manifesto:
                    for key, value in self._manifesto['get'].items():
                        echo["l"] = "get"
                        echo["w"] = key
                        echo["r"] = value['description']
                        self.talk(echo)
            case "run":
                if "w" in message and 'run' in self._manifesto:
                    echo: Dict[str, Any] = {
                        "c": 'echo',
                        "t": message["f"],
                        "i": message["i"]
                    }
                    if message["w"] in self._manifesto['run']:
                        echo["r"] = "ROGER"
                        self.talk(echo)
                        ok: bool = self._manifesto['run'][message["w"]]['function'](message)
                        if ok:
                            echo["r"] = "OK"
                        else:
                            echo["r"] = "FAIL"
                        self.talk(echo)
                    else:
                        echo["r"] = "UNKNOWN"
                        self.talk(echo)
            case "set":
                if "v" in message and isinstance(message["v"], int) and "w" in message and 'set' in self._manifesto:
                    echo: Dict[str, Any] = {
                        "c": 'echo',
                        "t": message["f"],
                        "i": message["i"]
                    }
                    if message["w"] in self._manifesto['set']:
                        echo["r"] = "ROGER"
                        self.talk(echo)
                        ok: bool = self._manifesto['set'][message["w"]]['function'](message, message["v"])
                        if ok:
                            echo["r"] = "OK"
                        else:
                            echo["r"] = "FAIL"
                        self.talk(echo)
                    else:
                        echo["r"] = "UNKNOWN"
                        self.talk(echo)
            case "get":
                if "w" in message and 'get' in self._manifesto:
                    echo: Dict[str, Any] = {
                        "c": 'echo',
                        "t": message["f"],
                        "i": message["i"]
                    }
                    if message["w"] in self._manifesto['get']:
                        echo["r"] = "ROGER"
                        self.talk(echo)
                        echo["r"] = "OK"
                        echo["v"] = self._manifesto['get'][message["w"]]['function'](message)
                        self.talk(echo)
                    else:
                        echo["r"] = "UNKNOWN"
                        self.talk(echo)
            case "sys":
                echo: Dict[str, Any] = {
                    "c": 'echo',
                    "t": message["f"],
                    "i": message["i"]
                }
                echo["r"] = f"{platform.platform()}"
                self.talk(echo)
            case "echo":
                if self._last_message and message["i"] == self._last_message["i"]:
                    if 'echo' in self._manifesto:
                        self._manifesto['echo'](message)
            case _:
                print("\tUnknown command type!")
        return False

    def validate_talk(self, talk: Dict[str, Any]) -> bool:
        if isinstance(talk, dict) and "s" in talk and "m" in talk:
            try:
                message_checksum: int = int(talk.get("s", None))
                print(f"\tLocal checksum: {JsonTalkie.checksum(talk["m"])}")
            except (ValueError, TypeError):
                return False
            if message_checksum == JsonTalkie.checksum(talk["m"]):
                message: int = talk["m"]
                if "c" in message and "f" in message and "i" in message:
                    if "t" in message:
                        return message["t"] == "*" or message["t"] == self._manifesto['talker']['name']
                    else:
                        return message["c"] == "talk" or message["c"] == "sys"
        return False


    @staticmethod
    def message_id() -> str:
        """Creates a unique message ID"""
        return uuid.uuid4().hex[:8]
    
    @staticmethod
    def encode(talk: Dict[str, Any]) -> bytes:
        # If specified, separators should be an (item_separator, key_separator)
        #     tuple. The default is (', ', ': ') if indent is None and
        #     (',', ': ') otherwise. To get the most compact JSON representation,
        #     you should specify (',', ':') to eliminate whitespace.
        return json.dumps(talk, separators=(',', ':')).encode('utf-8')

    @staticmethod
    def decode(data: bytes) -> Dict[str, Any]:
        data_str = data.decode('utf-8')
        try:
            data_dict = json.loads(data_str)
            return data_dict
        except (json.JSONDecodeError):
            return None

    @staticmethod
    def checksum(message: Dict[str, Any]) -> int:
        # If specified, separators should be an (item_separator, key_separator)
        #     tuple. The default is (', ', ': ') if indent is None and
        #     (',', ': ') otherwise. To get the most compact JSON representation,
        #     you should specify (',', ':') to eliminate whitespace.
        data = json.dumps(message, separators=(',', ':')).encode('utf-8')
        # 16-bit word and XORing
        checksum = 0
        for i in range(0, len(data), 2):
            # Combine two bytes into 16-bit value
            chunk = data[i] << 8
            if i+1 < len(data):
                chunk |= data[i+1]
            checksum ^= chunk
        return checksum & 0xFFFF

