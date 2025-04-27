from socket_udp import *

# Example usage
if __name__ == "__main__":
    # Create a broadcaster instance
    broadcaster = Broadcaster(device_name="MyDevice")
    broadcaster.start()
    
    try:
        # Example: Send discovery and get responses
        print("Discovering devices...")
        devices = broadcaster.discover_devices()
        print(f"Found {len(devices)} devices:")
        for device in devices:
            print(f"- {device['device_name']} ({device['device_id']})")
            
        # Example: Send custom message
        broadcaster.send_broadcast({
            'type': 'custom_message',
            'content': 'Hello network!',
            'sender': broadcaster.device_name,
            'timestamp': time.time()
        })
        
        # Keep running to receive messages
        while True:
            time.sleep(1)
            
    except KeyboardInterrupt:
        print("Shutting down...")
        broadcaster.stop()