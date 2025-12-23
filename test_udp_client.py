#!/usr/bin/env python3
"""
Simple UDP test client for WICOM ROS2 bridge.
This script can send basic commands to test the bridge without the VR client.
"""

import socket
import struct
import time
import sys

# Message IDs
MSG_ID_COMMAND = 10
MSG_ID_POSITION_CONTROL = 11
MSG_ID_VELOCITY_CONTROL = 12
MSG_ID_STATE = 1
MSG_ID_POSITION_FEEDBACK = 2
MSG_ID_DRONE_STATUS = 3

# Command IDs
CMD_ARM_DISARM = 1
CMD_SET_MODE = 2
CMD_TAKEOFF = 3
CMD_LAND = 4
CMD_POSITION_CONTROL_MODE = 5
CMD_VELOCITY_CONTROL_MODE = 8

# Modes
MODE_OFFBOARD = 4

class UAVLinkClient:
    def __init__(self, host='localhost', port=12345):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(1.0)
        print(f"UAVLink client initialized for {host}:{port}")
    
    def send_command(self, cmd_id, param1=0.0, param2=0.0, param3=0.0, param4=0.0):
        """Send a command message."""
        # Header: msgid (1 byte) + length (1 byte)
        # Payload: cmd_id (1 byte) + 4 floats (4 bytes each)
        buffer = struct.pack('!BB B ffff', 
                            MSG_ID_COMMAND, 19,  # Header
                            cmd_id,               # Command ID
                            param1, param2, param3, param4)  # Parameters
        self.sock.sendto(buffer, (self.host, self.port))
        print(f"Sent command: {cmd_id}, params: [{param1}, {param2}, {param3}, {param4}]")
    
    def send_position(self, x, y, z, yaw):
        """Send a position control message."""
        buffer = struct.pack('!BB ffff',
                            MSG_ID_POSITION_CONTROL, 16,
                            x, y, z, yaw)
        self.sock.sendto(buffer, (self.host, self.port))
        print(f"Sent position: x={x}, y={y}, z={z}, yaw={yaw}")
    
    def send_velocity(self, vx, vy, vz, yaw_rate):
        """Send a velocity control message."""
        buffer = struct.pack('!BB ffff',
                            MSG_ID_VELOCITY_CONTROL, 16,
                            vx, vy, vz, yaw_rate)
        self.sock.sendto(buffer, (self.host, self.port))
        print(f"Sent velocity: vx={vx}, vy={vy}, vz={vz}, yaw_rate={yaw_rate}")
    
    def receive(self, timeout=1.0):
        """Try to receive a message from the bridge."""
        self.sock.settimeout(timeout)
        try:
            data, addr = self.sock.recvfrom(1024)
            if len(data) >= 2:
                msgid = data[0]
                length = data[1]
                
                if msgid == MSG_ID_DRONE_STATUS and len(data) >= 60:
                    self.parse_drone_status(data)
                elif msgid == MSG_ID_POSITION_FEEDBACK and len(data) >= 15:
                    self.parse_position_feedback(data)
                elif msgid == MSG_ID_STATE and len(data) >= 16:
                    self.parse_state(data)
                else:
                    print(f"Received unknown message: ID={msgid}, length={length}")
            return True
        except socket.timeout:
            return False
    
    def parse_drone_status(self, data):
        """Parse drone status message."""
        # Skip header (2 bytes)
        x, y, z, vx, vy, vz, roll, pitch, yaw, battery = struct.unpack('!ffffffffff', data[2:42])
        armed = data[42]
        mode = data[43]
        range_front, range_back, range_left, range_right = struct.unpack('!ffff', data[44:60])
        
        print("\n=== DRONE STATUS ===")
        print(f"Position: x={x:.2f}, y={y:.2f}, z={z:.2f}")
        print(f"Velocity: vx={vx:.2f}, vy={vy:.2f}, vz={vz:.2f}")
        print(f"Attitude: roll={roll:.2f}, pitch={pitch:.2f}, yaw={yaw:.2f}")
        print(f"Battery: {battery:.2f}V")
        print(f"Armed: {armed}, Mode: {mode}")
        print(f"Range sensors: F={range_front:.2f}, B={range_back:.2f}, L={range_left:.2f}, R={range_right:.2f}")
        print("====================\n")
    
    def parse_position_feedback(self, data):
        """Parse position feedback message."""
        x, y, z = struct.unpack('!fff', data[2:14])
        success = data[14]
        print(f"Position feedback: x={x:.2f}, y={y:.2f}, z={z:.2f}, success={success}")
    
    def parse_state(self, data):
        """Parse state message."""
        roll, pitch, yaw = struct.unpack('!fff', data[2:14])
        armed = data[14]
        mode = data[15]
        print(f"State: roll={roll:.2f}, pitch={pitch:.2f}, yaw={yaw:.2f}, armed={armed}, mode={mode}")
    
    def arm(self):
        """Send arm command."""
        self.send_command(CMD_ARM_DISARM, 1.0)
    
    def disarm(self):
        """Send disarm command."""
        self.send_command(CMD_ARM_DISARM, 0.0)
    
    def set_offboard_mode(self):
        """Send command to set offboard mode."""
        self.send_command(CMD_SET_MODE, MODE_OFFBOARD)
    
    def takeoff(self, altitude):
        """Send takeoff command."""
        self.send_command(CMD_TAKEOFF, altitude)
    
    def land(self):
        """Send land command."""
        self.send_command(CMD_LAND)
    
    def enable_position_control(self):
        """Enable position control mode."""
        self.send_command(CMD_POSITION_CONTROL_MODE, 1.0)
    
    def disable_position_control(self):
        """Disable position control mode."""
        self.send_command(CMD_POSITION_CONTROL_MODE, 0.0)
    
    def enable_velocity_control(self):
        """Enable velocity control mode."""
        self.send_command(CMD_VELOCITY_CONTROL_MODE, 1.0)
    
    def disable_velocity_control(self):
        """Disable velocity control mode."""
        self.send_command(CMD_VELOCITY_CONTROL_MODE, 0.0)
    
    def close(self):
        """Close the socket."""
        self.sock.close()

def test_sequence(client):
    """Run a test sequence."""
    print("\n=== Starting test sequence ===\n")
    
    # Wait for initial connection
    print("Step 1: Waiting for drone status...")
    for _ in range(10):
        if client.receive(timeout=1.0):
            break
        time.sleep(0.5)
    
    print("\nStep 2: Sending takeoff command (altitude=5m)")
    client.takeoff(5.0)
    time.sleep(1.0)
    
    print("\nStep 3: Sending arm command")
    client.arm()
    time.sleep(1.0)
    
    print("\nStep 4: Setting offboard mode")
    client.set_offboard_mode()
    time.sleep(1.0)
    
    print("\nStep 5: Enabling position control")
    client.enable_position_control()
    time.sleep(1.0)
    
    print("\nStep 6: Sending position command (2m east, 2m north, 5m up)")
    client.send_position(2.0, 2.0, 5.0, 0.0)
    time.sleep(2.0)
    
    # Listen for status updates
    print("\nStep 7: Listening for status updates (10 seconds)...")
    start_time = time.time()
    while time.time() - start_time < 10.0:
        client.receive(timeout=0.5)
    
    print("\nStep 8: Landing")
    client.land()
    time.sleep(2.0)
    
    print("\n=== Test sequence complete ===\n")

def interactive_mode(client):
    """Interactive command mode."""
    print("\n=== Interactive Mode ===")
    print("Commands:")
    print("  arm         - Arm the vehicle")
    print("  disarm      - Disarm the vehicle")
    print("  offboard    - Set offboard mode")
    print("  takeoff <h> - Takeoff to altitude h")
    print("  land        - Land")
    print("  pos <x> <y> <z> <yaw> - Send position setpoint")
    print("  vel <vx> <vy> <vz> <yr> - Send velocity setpoint")
    print("  enable_pos  - Enable position control")
    print("  disable_pos - Disable position control")
    print("  enable_vel  - Enable velocity control")
    print("  disable_vel - Disable velocity control")
    print("  listen      - Listen for messages (10s)")
    print("  quit        - Exit")
    print()
    
    while True:
        try:
            cmd = input("> ").strip().split()
            if not cmd:
                continue
            
            if cmd[0] == 'quit':
                break
            elif cmd[0] == 'arm':
                client.arm()
            elif cmd[0] == 'disarm':
                client.disarm()
            elif cmd[0] == 'offboard':
                client.set_offboard_mode()
            elif cmd[0] == 'takeoff' and len(cmd) >= 2:
                client.takeoff(float(cmd[1]))
            elif cmd[0] == 'land':
                client.land()
            elif cmd[0] == 'pos' and len(cmd) >= 5:
                client.send_position(float(cmd[1]), float(cmd[2]), 
                                    float(cmd[3]), float(cmd[4]))
            elif cmd[0] == 'vel' and len(cmd) >= 5:
                client.send_velocity(float(cmd[1]), float(cmd[2]), 
                                    float(cmd[3]), float(cmd[4]))
            elif cmd[0] == 'enable_pos':
                client.enable_position_control()
            elif cmd[0] == 'disable_pos':
                client.disable_position_control()
            elif cmd[0] == 'enable_vel':
                client.enable_velocity_control()
            elif cmd[0] == 'disable_vel':
                client.disable_velocity_control()
            elif cmd[0] == 'listen':
                print("Listening for 10 seconds...")
                start = time.time()
                while time.time() - start < 10.0:
                    client.receive(timeout=0.5)
            else:
                print("Unknown command or wrong number of arguments")
        except KeyboardInterrupt:
            print("\nExiting...")
            break
        except Exception as e:
            print(f"Error: {e}")

def main():
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python3 test_udp_client.py <host> [mode]")
        print()
        print("Arguments:")
        print("  host - IP address of the bridge (e.g., localhost or 192.168.1.100)")
        print("  mode - 'test' for automated test sequence or 'interactive' (default)")
        print()
        print("Examples:")
        print("  python3 test_udp_client.py localhost")
        print("  python3 test_udp_client.py 192.168.1.100 test")
        sys.exit(1)
    
    host = sys.argv[1]
    mode = sys.argv[2] if len(sys.argv) >= 3 else 'interactive'
    
    client = UAVLinkClient(host)
    
    try:
        if mode == 'test':
            test_sequence(client)
        else:
            interactive_mode(client)
    finally:
        client.close()
        print("Client closed")

if __name__ == '__main__':
    main()
