import json
import threading
import uuid
from typing import Dict, Any

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
        case "list":
            return JsonTalkie.send_json({
                'description': 'This device does a 500ms buzz!'
            })

    return False


def call(function: str) -> bool:
    match function:
        case "buzz":
            buzz()

    return True


class WalkieDevice:
    """Device with managed socket lifecycle."""

    def __init__(self, talkie: JsonTalkie, device_name: str = None):
        self._talkie = talkie  # Composition over inheritance
        self._device_id = str(uuid.uuid4())
        self._name = device_name or f"Device-{self._device_id[:8]}"
    
    def start(self) -> bool:
        return self._talkie.on(self)
    
    def stop(self):
        return self._talkie.off()
    
    def talk(self, message: Dict[str, Any]) -> bool:
        """Sends messages without network awareness."""
        return self._talkie.send_json( message )
    
    def roger(self, message: Dict[str, Any]) -> Dict[str, Any]:
        """Override this to handle business logic."""
        print(f"[{self._name}] Received: {message}")
        return {
            'roger': 'Roger'
        }
