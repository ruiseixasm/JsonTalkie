import socket
from typing import Optional, Tuple

from broadcast_socket import BroadcastSocket

class BroadcastSocket_UDP(BroadcastSocket):
    """UDP broadcast socket with explicit lifecycle control."""
    
    def __init__(self, port: int = 5005, broadcast_addr: str = '255.255.255.255'):
        self._port = port
        self._broadcast_addr = broadcast_addr
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
            self._socket.sendto(data, (self._broadcast_addr, self._port))
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
        

