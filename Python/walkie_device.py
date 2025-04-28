import json
import threading
import uuid
from typing import Dict, Any

from broadcast_socket import BroadcastSocket
from json_talkie import JsonTalkie

import numpy as np
import simpleaudio as sa

def buzz(duration_ms=500, freq=1000, volume=0.5):
    """High-quality buzzer sound"""
    sample_rate = 44100  # CD quality
    t = np.linspace(0, duration_ms/1000, int(sample_rate * duration_ms/1000), False)
    wave = np.sin(2 * np.pi * freq * t) * volume
    
    # Convert to 16-bit PCM
    audio = (wave * 32767).astype(np.int16)
    play_obj = sa.play_buffer(audio, 1, 2, sample_rate)
    play_obj.wait_done()


def process(message: Dict[str, Any]) -> bool:
    match message['command']:
        case "call":
            return JsonTalkie.call(message['function'])

    return False


def call(function: str) -> bool:
    match function:
        case "buzz":
            buzz()

    return True


class WalkieDevice:
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
            message_talkie: Dict[str, Any] = json.loads(data.decode('utf-8'))
            if JsonTalkie.check_message(message_talkie):
                self.on_message(message_talkie['message'])
        except (UnicodeDecodeError, json.JSONDecodeError) as e:
            print(f"[{self._name}] Invalid message: {e}")
    
    def send_json(self, message: Dict[str, Any]) -> bool:
        """Sends messages without network awareness."""
        message_talkie: Dict[str, Any] = {
            'checksum': JsonTalkie.checksum_8bit( json.dumps(message) ),
            'message': message
        }
        return self._socket.send( json.dumps(message_talkie).encode('utf-8') )
    
    def on_message(self, message: Dict[str, Any]):
        """Override this to handle business logic."""
        print(f"[{self._name}] Received: {message}")

