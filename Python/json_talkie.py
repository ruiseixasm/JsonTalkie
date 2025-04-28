import json
import threading
import uuid
from typing import Dict, Any


class JsonTalkie:

    def checksum_8bit(message: str) -> int:
        """Lightweight checksum suitable for microcontrollers"""
        checksum = 0
        for char in message:
            checksum ^= ord(char)  # XOR each character
        return checksum % 256  # Ensure 8-bit value

    def check_message(message_talkie: Dict[str, Any]) -> bool:
        checksum_talkie: int = message_talkie['checksum']
        return checksum_talkie == JsonTalkie.checksum_8bit( json.dumps(message_talkie['message']) )

