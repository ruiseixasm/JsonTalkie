import json
import threading
import uuid
from typing import Dict, Any, TYPE_CHECKING

from broadcast_socket import BroadcastSocket


class JsonTalkie:

    def __init__(self, socket: BroadcastSocket):
        from walkie_device import WalkieDevice
        self._socket: BroadcastSocket = socket  # Composition over inheritance
        self._running: bool = False
        self._walkie: WalkieDevice = None


    if TYPE_CHECKING:
        from walkie_device import WalkieDevice

    def on(self, walkie: 'WalkieDevice') -> bool:
        """Start message processing (no network knowledge)."""
        if not self._socket.open():
            return False
        self._running = True
        self._walkie = walkie
        self._thread = threading.Thread(target=self._listen_loop, daemon=True)
        self._thread.start()
        return True
    
    def off(self):
        """Stop processing (delegates cleanup to socket)."""
        self._running = False
        self._walkie = None
        if hasattr(self, '_thread'):
            self._thread.join()
        self._socket.close()
    
    def send_json(self, message: Dict[str, Any]) -> bool:
        """Sends messages without network awareness."""
        message_talkie: Dict[str, Any] = {
            'checksum': JsonTalkie.checksum_8bit( json.dumps(message) ),
            'message': message
        }
        return self._socket.send( json.dumps(message_talkie).encode('utf-8') )
    
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
            message_talkie: Dict[str, Any] = json.loads(data.decode('utf-8'))
            if JsonTalkie.check_message(message_talkie):
                self._walkie.on_message(message_talkie['message'])
        except (UnicodeDecodeError, json.JSONDecodeError) as e:
            print(f"[{self._walkie._name}] Invalid message: {e}")


    @staticmethod
    def checksum_8bit(message: str) -> int:
        """Lightweight checksum suitable for microcontrollers"""
        checksum = 0
        for char in message:
            checksum ^= ord(char)  # XOR each character
        return checksum % 256  # Ensure 8-bit value

    @staticmethod
    def check_message(message_talkie: Dict[str, Any]) -> bool:
        checksum_talkie: int = message_talkie['checksum']
        return checksum_talkie == JsonTalkie.checksum_8bit( json.dumps(message_talkie['message']) )

