import socket
import json
import threading
import time
import uuid


class Broadcaster:
    def __init__(self, port=5005, broadcast_address='255.255.255.255', device_name=None):
        self.port = port
        self.broadcast_address = broadcast_address
        self.device_id = str(uuid.uuid4())
        self.device_name = device_name or f"Device-{self.device_id[:8]}"
        self.running = False
        
        # Create UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.sock.settimeout(0.2)  # Small timeout for receive
        
    def start(self):
        """Start the broadcast listener and responder"""
        self.running = True
        self.listener_thread = threading.Thread(target=self._listen_for_messages)
        self.listener_thread.daemon = True
        self.listener_thread.start()
        print(f"{self.device_name} started listening on port {self.port}")
        
    def stop(self):
        """Stop the broadcast listener"""
        self.running = False
        self.listener_thread.join()
        
    def _listen_for_messages(self):
        """Listen for incoming broadcast messages"""
        while self.running:
            try:
                data, addr = self.sock.recvfrom(4096)
                try:
                    message = json.loads(data.decode('utf-8'))
                    self._handle_message(message, addr)
                except json.JSONDecodeError:
                    print(f"Received invalid JSON from {addr}")
            except socket.timeout:
                continue
            except Exception as e:
                print(f"Error receiving message: {e}")
                continue
    
    def _handle_message(self, message, addr):
        """Process received JSON message and optionally respond"""
        print(f"Received from {addr}: {message}")
        
        # Example response logic - customize for your needs
        if message.get('type') == 'discovery':
            response = {
                'type': 'discovery_response',
                'device_id': self.device_id,
                'device_name': self.device_name,
                'timestamp': time.time()
            }
            self.send_broadcast(response)
            
    def send_broadcast(self, data):
        """Broadcast a JSON message to the network"""
        try:
            json_data = json.dumps(data).encode('utf-8')
            self.sock.sendto(json_data, (self.broadcast_address, self.port))
            print(f"Broadcasted: {data}")
        except Exception as e:
            print(f"Error broadcasting message: {e}")
            
    def discover_devices(self, timeout=3):
        """Send a discovery request and collect responses"""
        responses = []
        
        def collect_responses():
            start_time = time.time()
            while time.time() - start_time < timeout:
                try:
                    data, addr = self.sock.recvfrom(4096)
                    message = json.loads(data.decode('utf-8'))
                    if message.get('type') == 'discovery_response':
                        responses.append(message)
                except (socket.timeout, json.JSONDecodeError):
                    continue
        
        # Start listener thread
        listener = threading.Thread(target=collect_responses)
        listener.daemon = True
        listener.start()
        
        # Send discovery request
        self.send_broadcast({
            'type': 'discovery',
            'device_id': self.device_id,
            'device_name': self.device_name,
            'timestamp': time.time()
        })
        
        # Wait for responses
        listener.join(timeout)
        return responses

