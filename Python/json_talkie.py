import json
import threading
import uuid
from typing import Dict, Any, TYPE_CHECKING
import time

from broadcast_socket import BroadcastSocket


class JsonTalkie:

    def __init__(self, socket: BroadcastSocket):
        from walkie_device import WalkieDevice
        self._socket: BroadcastSocket = socket  # Composition over inheritance
        self._running: bool = False
        self._walkie: WalkieDevice = None
        self._last_message: Dict[str, Any] = {}
        self._message_time: float = 0.0


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

    def talk(self, message: Dict[str, Any]) -> bool:
        """Sends messages without network awareness."""
        if self._walkie:
            message['from'] = self._walkie._name
            message['id'] = self.generate_message_id()
            talk: Dict[str, Any] = {
                'checksum': JsonTalkie.checksum_16bit_bytes( json.dumps(message).encode('utf-8') ),
                'message': message
            }
            self._last_message = message
            self._message_time = time.time()
            return self._socket.send( json.dumps(talk).encode('utf-8') )
        return False
    
    def wait(self, seconds: float = 2) -> bool:
        return self._last_message and time.time() - self._message_time < seconds
    
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
            talk: Dict[str, Any] = json.loads(data.decode('utf-8'))
            if JsonTalkie.validate_talk(talk):
                if self._walkie._name != talk['message']['from']: # Makes sure sender doesn't process it's own messages

                    match talk['message']['talk']:
                        case "echo":
                            if talk['message']['id'] == self._last_message['id']:
                                print(f"[{self._walkie._name}] Acknowledge")
                                self._last_message = {}
                        case _:
                            self._walkie.roger(talk['message'])
        except (UnicodeDecodeError, json.JSONDecodeError) as e:
            print(f"[{self._walkie._name}] Invalid message: {e}")


    @staticmethod
    def generate_message_id() -> str:
        """Creates a unique message ID combining timestamp and UUID"""
        # timestamp: str = hex(int(time.time() * 1000))[2:]  # Millisecond precision
        message_id: str = uuid.uuid4().hex[:8]  # First 8 chars of UUID
        # return f"{timestamp}-{message_id}"
        return message_id
    
    @staticmethod
    def checksum_8bit(message: str) -> int:
        """Lightweight checksum suitable for microcontrollers"""
        checksum = 0
        for char in message:
            checksum ^= ord(char)  # XOR each character
        return checksum % 256  # Ensure 8-bit value

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
    def check_talk(talk: Dict[str, Any]) -> bool:
        if 'checksum' in talk and 'message' in talk:
            message_checksum: int = talk['checksum']
            return message_checksum == JsonTalkie.checksum_16bit_bytes( json.dumps(talk['message']).encode('utf-8') )
        return False

    @staticmethod
    def validate_talk(talk: Dict[str, Any]) -> bool:
        if JsonTalkie.check_talk(talk):
            if 'talk' in talk['message'] and 'from' in talk['message'] and 'id' in talk['message']:
                return True
        
        return False

