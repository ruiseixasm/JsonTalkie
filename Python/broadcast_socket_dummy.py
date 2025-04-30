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
import socket
import json
import threading
import uuid
import random
import time
from typing import Optional, Tuple, Any, Dict

from broadcast_socket import BroadcastSocket

class BroadcastSocket_Dummy(BroadcastSocket):
    """Dummy broadcast socket with explicit lifecycle control."""
    
    def __init__(self, port: int = 5005):
        self._port = port
        self._socket = None  # Not initialized until open()
        self._time: float = time.time()
        self._sent_message: Dict[str, Any] = {}
    
    def open(self) -> bool:
        """Initialize and bind the socket."""
        try:
            divide: float = 1/random.randint(0, 1000)
            self._socket = True
            return True
        except Exception as e:
            print(f"Socket open failed: {e}")
            return False
    
    def close(self):
        """Release socket resources."""
        if self._socket:
            self._socket = None
    
    def send(self, data: bytes) -> bool:
        """Broadcast data if socket is active."""
        if not self._socket:
            return False
        try:
            divide: float = 1/random.randint(0, 1000)
            print(f"DUMMY SENT: {data}")
            talk: Dict[str, Any] = BroadcastSocket_Dummy.decode(data)
            message: Dict[str, Any] = talk['message']
            self._sent_message = message
            return True
        except Exception as e:
            print(f"DUMMY Send failed: {e}")
            return False
    
    def receive(self) -> Optional[Tuple[bytes, Tuple[str, int]]]:
        """Non-blocking receive."""
        if not self._socket:
            return None
        try:
            if time.time() - self._time > 1:
                self._time = time.time()
                random_number: int = random.randint(0, 1000)
                if random.randint(0, 1000) < 10:
                    divide: float = 1/random_number
                    message = self.messages[random_number % len(self.messages)]
                    message['id'] = BroadcastSocket_Dummy.message_id()
                    checksum = BroadcastSocket_Dummy.checksum(message)
                    talk: Dict[str, Any] = {
                        'checksum': checksum,
                        'message': message
                    }
                    data = BroadcastSocket_Dummy.encode(talk)
                    print(f"DUMMY RECEIVED: {data}")
                    return data
            return None
        except BlockingIOError:
            return None
        except Exception as e:
            print(f"DUMMY Receive error: {e}")
            return None

    messages: tuple[Dict[str, Any]] = (
        {"type": "run", "what": "buzz", "to": "Buzzer", "from": "Buzzer", "id": "bc40fd17"},
        {"type": "echo", "to": "Buzzer", "id": "bc40fd17", "response": "[Buzzer buzz]\\tCalled", "from": "Buzzer"},
        {"type": "talk", "from": "Dummy", "id": "dce4fac7"},
        {"type": "echo", "to": "Talker-a6", "id": "dce4fac7", "response": "[Talker-a6]\\tA simple Talker!", "from": "Talker-a6"}
    )

    @staticmethod
    def message_id() -> str:
        """Creates a unique message ID combining timestamp and UUID"""
        # timestamp: str = hex(int(time.time() * 1000))[2:]  # Millisecond precision
        # id: str = uuid.uuid4().hex[:8]  # First 8 chars of UUID
        # return f"{timestamp}-{id}"
        return uuid.uuid4().hex[:8]
    
    @staticmethod
    def checksum_16bit_bytes(data: bytes) -> int:
        """16-bit XOR checksum for bytes"""
        checksum = 0
        for i in range(0, len(data), 2):
            # Combine two bytes into 16-bit value
            chunk = data[i] << 8
            if i+1 < len(data):
                chunk |= data[i+1]
            checksum ^= chunk
        return checksum & 0xFFFF

    @staticmethod
    def encode(talk: Dict[str, Any]) -> bytes:
        return json.dumps(talk).encode('utf-8')

    @staticmethod
    def decode(data: bytes) -> Dict[str, Any]:
        return data.decode('utf-8')

    @staticmethod
    def checksum(message: Dict[str, Any]) -> int:
        data = json.dumps(message).encode('utf-8')
        return BroadcastSocket_Dummy.checksum_16bit_bytes(data)
