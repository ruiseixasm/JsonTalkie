import json
import threading
import uuid
from typing import Dict, Any

from broadcast_socket import BroadcastSocket

class JsonDevice:
    """Device with managed socket lifecycle."""
    
    def __init__(self, socket: BroadcastSocket, device_name: str = None):
        self._socket = socket
        self._device_id = str(uuid.uuid4())
        self._name = device_name or f"Device-{self._device_id[:8]}"
        self._running = False
    
    def start(self) -> bool:
        """Begin listening (opens socket)."""
        if not self._socket.open():
            return False
        self._running = True
        self._thread = threading.Thread(target=self._listen_loop, daemon=True)
        self._thread.start()
        return True
    
    def stop(self):
        """Stop listening (closes socket)."""
        self._running = False
        if hasattr(self, '_thread'):
            self._thread.join()
        self._socket.close()
    
    def _listen_loop(self):
        """Main receive loop."""
        while self._running:
            data = self._socket.receive()
            if data:
                self._handle_message(*data)
    
    def _handle_message(self, data: bytes, ip: str, port: int):
        """Process incoming messages."""
        try:
            message = json.loads(data.decode('utf-8'))
            self.on_message(message, ip, port)
        except (UnicodeDecodeError, json.JSONDecodeError) as e:
            print(f"[{self._name}] Invalid message: {e}")
    
    def send_json(self, message: Dict[str, Any]) -> bool:
        """Broadcast JSON (fails if socket not open)."""
        return self._socket.send(json.dumps(message).encode('utf-8'))
    
    def on_message(self, message: Dict[str, Any], ip: str, port: int):
        """Override for custom message handling."""
        print(f"[{self._name}] From {ip}:{port}: {message}")

