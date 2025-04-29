import json
import threading
import uuid
from typing import Dict, Any, TYPE_CHECKING, Callable
import time

from broadcast_socket import BroadcastSocket


class JsonTalkie:

    def __init__(self, socket: BroadcastSocket, manifesto: Dict[str, Any]):
        self._socket: BroadcastSocket = socket  # Composition over inheritance
        self._manifesto: Dict[str, Any] = manifesto
        self._talker_name: str = self._manifesto['talk']['name']
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
        message['id'] = self.generate_message_id()
        talk: Dict[str, Any] = {
            'checksum': JsonTalkie.checksum_16bit_bytes( json.dumps(message).encode('utf-8') ),
            'message': message
        }
        return self._socket.send( json.dumps(talk).encode('utf-8') )
    
    def listen(self):
        """Processes raw bytes from socket."""
        while self._running:
            received = self._socket.receive()
            if received:
                data, _ = received  # Explicitly ignore (ip, port)
                try:
                    talk: Dict[str, Any] = json.loads(data.decode('utf-8'))
                    if self.validate_talk(talk):
                        self.receive(talk['message'])
                except (UnicodeDecodeError, json.JSONDecodeError) as e:
                    print(f"Invalid message: {e}")

    def receive(self, message: Dict[str, Any]) -> bool:
        """Handles message content only."""
        match message['talk']:
            case "talk":
                print(f"[{self._manifesto['talk']['name']}] \t {self._manifesto['talk']['description']}")
            case "call":
                function = self._manifesto['list']['call'][message['function']]['function']
                function()
            case "echo":
                if message['id'] == last_message['id']:
                    match last_message['talk']:
                        case "list":
                            print(f"[{message['from']}] Listed")
                        case "call":
                            print(f"[{message['from']}] Executed")
                            last_message = {}
            case _:
                print("Unknown talking!")
        return False

    def wait(self, seconds: float = 2) -> bool:
        return self._last_message and time.time() - self._message_time < seconds



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

    def validate_talk(self, talk: Dict[str, Any]) -> bool:
        if 'checksum' in talk and 'message' in talk:
            message_checksum: int = talk['checksum']
            if message_checksum == JsonTalkie.checksum_16bit_bytes( json.dumps(talk['message']).encode('utf-8') ):
                message: int = talk['message']
                if 'talk' in message and 'from' in message and 'id' in message:
                    return 'to' in message and message['to'] == self._talker_name or message['talk'] == "talk"
        return False

