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
from typing import Optional, Tuple

from broadcast_socket import BroadcastSocket

class BroadcastSocket_UDP(BroadcastSocket):
    """UDP broadcast socket with explicit lifecycle control."""
    
    def __init__(self, port: int = 5005):
        self._port = port
        self._socket = None  # Not initialized until open()
    
    def open(self) -> bool:
        """Initialize and bind the socket."""
        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  # Critical!
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            self._socket.bind(('', self._port))
            self._socket.setblocking(False)
            return True
        except Exception as e:
            print(f"Socket open failed: {e}")
            return False
    
    def close(self):
        """Release socket resources."""
        if self._socket:
            self._socket.close()
            self._socket = None
    
    def send(self, data: bytes) -> bool:
        """Broadcast data if socket is active."""
        if not self._socket:
            return False
        try:
            self._socket.sendto(data, ('192.168.31.255', self._port))
            return True
        except Exception as e:
            print(f"Send failed: {e}")
            return False
    
    def receive(self) -> Optional[Tuple[bytes, Tuple[str, int]]]:
        """Non-blocking receive."""
        if not self._socket:
            return None
        try:
            return self._socket.recvfrom(4096)
        except BlockingIOError:
            return None
        except Exception as e:
            print(f"Receive error: {e}")
            return None
        

