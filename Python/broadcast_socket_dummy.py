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
import random
import time
from typing import Optional, Tuple, Any

from broadcast_socket import BroadcastSocket

class BroadcastSocket_Dummy(BroadcastSocket):
    """Dummy broadcast socket with explicit lifecycle control."""
    
    def __init__(self, port: int = 5005):
        self._port = port
        self._socket = None  # Not initialized until open()
        self._time: float = time.time()
        self._last_talker: str = "Buzzer"
    
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
            return True
        except Exception as e:
            print(f"Send failed: {e}")
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
                    data = self.receives[random_number % len(self.receives)]
                    talk = data.decode('utf-8')
                    
                    print(f"DUMMY RECEIVED: {data}")
                    return data
            return None
        except BlockingIOError:
            return None
        except Exception as e:
            print(f"Receive error: {e}")
            return None
        
    receives: tuple[tuple] = [
        (b'{"checksum": 7018, "message": {"type": "run", "what": "buzz", "to": "Buzzer", "from": "Buzzer", "id": "bc40fd17"}}', ('192.168.31.22', 5005)),
        (b'{"checksum": 23366, "message": {"type": "echo", "to": "Buzzer", "id": "bc40fd17", "response": "[Buzzer buzz]\\tCalled", "from": "Buzzer"}}', ('192.168.31.22', 5005)),
        (b'{"checksum": 9331, "message": {"type": "talk", "from": "Talker-a6", "id": "dce4fac7"}}', ('192.168.31.22', 5005)),
        (b'{"checksum": 31299, "message": {"type": "echo", "to": "Talker-a6", "id": "dce4fac7", "response": "[Talker-a6]\\tA simple Talker!", "from": "Talker-a6"}}', ('192.168.31.22', 5005))
    ]

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
