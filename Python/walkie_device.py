import json
import threading
import uuid
from typing import Dict, Any, Callable
import time

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


_manifesto: Dict[str, Any] = {
    'talk': {
        'name': 'Buzzer',
        'description': 'This device does a 500ms buzz!'
    },
    'list': {
        'call': {
            'buzz': {
                'description': 'Triggers a 500ms buzzing sound',
                'function': buzz
            }
        }
    }
}


class WalkieDevice:
    """Device with managed socket lifecycle."""

    def __init__(self, talkie: JsonTalkie, device_name: str = None):
        self._talkie = talkie  # Composition over inheritance
        self._device_id = str(uuid.uuid4())
        self._name = device_name or f"Device-{self._device_id[:8]}"
        self._last_message: Dict[str, Any] = {}
        self._message_time: float = 0.0
    
    def start(self) -> bool:
        return self._talkie.on(self.receive)    # Where the receiver is set
    
    def stop(self):
        return self._talkie.off()
    
    def wait(self, seconds: float = 2) -> bool:
        return self._last_message and time.time() - self._message_time < seconds
    
    def send(self, message: Dict[str, Any]) -> bool:
        """Sends messages without network awareness."""
        if 'from' not in message:
            message['from'] = self._name
        self._last_message = message
        self._message_time = time.time()
        return self._talkie.talk( message )
    
    def receive(self, message: Dict[str, Any]) -> bool:
        """Handles message content only."""
        if WalkieDevice.validate_message(message):
            if message['from'] != self._name: # Makes sure sender doesn't process it's own messages
                match message['talk']:
                    case "echo":
                        if message['id'] == self._last_message['id']:
                            match self._last_message['talk']:
                                case "list":
                                    print(f"[{message['from']}] Listed")
                                case "call":
                                    print(f"[{message['from']}] Executed")
                                    self._last_message = {}
                    case _:
                        self.roger(message)
        return False

    def roger(self, message: Dict[str, Any]) -> Dict[str, Any]:
        """Override this to handle business logic."""
        print(f"[{self._name}] Received: {message}")
        return {
            'roger': 'Roger'
        }


    @staticmethod
    def validate_message(message: Dict[str, Any]) -> bool:
        if 'talk' in message and 'from' in message and 'id' in message:
            return message['talk'] == "talk" or 'to' in message
        return False
    
