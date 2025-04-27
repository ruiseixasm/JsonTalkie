import socket
from typing import Optional, Tuple

class BroadcastSocket:
    """UDP broadcast socket with explicit lifecycle control."""
    
    def __init__(self, port: int, broadcast_addr: str = '255.255.255.255'):
        self.port = port
        self.broadcast_addr = broadcast_addr
        self.sock = None  # Not initialized until open()
    
    def open(self) -> bool:
        """Initialize and bind the socket."""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            self.sock.bind(('', self.port))
            self.sock.setblocking(False)
            return True
        except Exception as e:
            print(f"Socket open failed: {e}")
            return False
    
    def close(self):
        """Release socket resources."""
        if self.sock:
            self.sock.close()
            self.sock = None
    
    def send(self, data: bytes) -> bool:
        """Broadcast data if socket is active."""
        if not self.sock:
            return False
        try:
            self.sock.sendto(data, (self.broadcast_addr, self.port))
            return True
        except Exception as e:
            print(f"Send failed: {e}")
            return False
    
    def receive(self) -> Optional[Tuple[bytes, Tuple[str, int]]]:
        """Non-blocking receive."""
        if not self.sock:
            return None
        try:
            return self.sock.recvfrom(4096)
        except BlockingIOError:
            return None
        except Exception as e:
            print(f"Receive error: {e}")
            return None
        

