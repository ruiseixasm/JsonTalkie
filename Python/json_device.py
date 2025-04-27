import json
import threading
import uuid
from typing import Dict, Any

from broadcast_socket import BroadcastSocket

class JsonDevice:
    """Device with managed socket lifecycle."""

    def __init__(self, socket: BroadcastSocket, device_name: str = None):
        self._socket = socket  # Composition over inheritance
        self._device_id = str(uuid.uuid4())
        self._name = device_name or f"Device-{self._device_id[:8]}"
        self._running = False
    
    def start(self) -> bool:
        """Start message processing (no network knowledge)."""
        if not self._socket.open():
            return False
        self._running = True
        self._thread = threading.Thread(target=self._listen_loop, daemon=True)
        self._thread.start()
        return True
    
    def stop(self):
        """Stop processing (delegates cleanup to socket)."""
        self._running = False
        if hasattr(self, '_thread'):
            self._thread.join()
        self._socket.close()
    
    def _listen_loop(self):
        """Processes raw bytes from socket."""
        while self._running:
            received = self._socket.receive()
            if received:
                data, _ = received  # Explicitly ignore (ip, port)
                self._handle_message(data)
    
    def _handle_message(self, data: bytes):
        """Handles message content only."""
        try:
            message = json.loads(data.decode('utf-8'))
            self.on_message(message)  # No IP/port exposed!
        except (UnicodeDecodeError, json.JSONDecodeError) as e:
            print(f"[{self._name}] Invalid message: {e}")
    
    def send_json(self, message: Dict[str, Any]) -> bool:
        """Sends messages without network awareness."""
        return self._socket.send(json.dumps(message).encode('utf-8'))
    
    def on_message(self, message: Dict[str, Any]):
        """Override this to handle business logic."""
        print(f"[{self._name}] Received: {message}")

