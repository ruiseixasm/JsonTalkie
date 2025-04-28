import json
import threading
import uuid
from typing import Dict, Any

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



class JsonTalkie:

    def checksum_8bit(message: str) -> int:
        """Lightweight checksum suitable for microcontrollers"""
        checksum = 0
        for char in message:
            checksum ^= ord(char)  # XOR each character
        return checksum % 256  # Ensure 8-bit value


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


