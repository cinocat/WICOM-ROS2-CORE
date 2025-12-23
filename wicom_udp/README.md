# WICOM UDP Bridge for ROS2

This package implements a UDP bridge for controlling PX4 drones via ROS2 Humble, maintaining compatibility with the existing VR client using the UAVLink protocol.

## Overview

The UDP bridge receives control commands from a VR client over UDP and translates them to PX4 commands via `px4_ros_com`. It also sends drone status back to the VR client periodically.

## Features

- UDP server on port 12345 (configurable)
- UAVLink protocol support (compatible with existing VR client)
- PX4 offboard control via px4_msgs
- Commands supported:
  - Arm/Disarm
  - Set mode (Offboard)
  - Takeoff
  - Land
  - Position control
  - Velocity control
- Periodic drone status updates
- Position command timeout watchdog

## Dependencies

- ROS2 Humble
- px4_msgs (from px4_ros_com)
- rclcpp
- std_msgs
- geometry_msgs
- sensor_msgs

## Building

```bash
cd ~/ros2_ws
colcon build --packages-select wicom_udp
source install/setup.bash
```

## Running

### Basic Launch

```bash
ros2 launch wicom_udp udp_server.launch.py
```

### Custom Port

```bash
ros2 launch wicom_udp udp_server.launch.py port:=12346
```

### Custom Config File

```bash
ros2 launch wicom_udp udp_server.launch.py config_file:=/path/to/config.yaml
```

## Configuration

Edit `config/udp_server.yaml` to customize:

- `port`: UDP port (default: 12345)
- `status_send_rate`: Rate for sending drone status (default: 0.5s)
- `position_timeout`: Timeout for position commands (default: 0.1s)
- `offboard_rate`: Rate for offboard setpoints (default: 0.05s = 20Hz)

## UAVLink Protocol

### Messages Sent to VR Client

1. **DRONE_STATUS** (ID: 3) - Periodic updates
   - Position (x, y, z)
   - Velocity (vx, vy, vz)
   - Orientation (roll, pitch, yaw)
   - Battery voltage
   - Armed status
   - Mode
   - Range sensors (front, back, left, right)

2. **POSITION_FEEDBACK** (ID: 2) - After position commands
   - Target position (x, y, z)
   - Success flag

3. **STATE** (ID: 1) - Vehicle state
   - Orientation (roll, pitch, yaw)
   - Armed status
   - Mode

### Messages Received from VR Client

1. **COMMAND** (ID: 10)
   - Arm/Disarm
   - Set mode
   - Takeoff
   - Land
   - Enable/disable position control

2. **POSITION_CONTROL** (ID: 11)
   - Target position (x, y, z, yaw)

3. **VELOCITY_CONTROL** (ID: 12)
   - Target velocity (vx, vy, vz, yaw_rate)

## PX4 Integration

The bridge uses the following PX4 topics:

### Subscriptions (from PX4)
- `/fmu/out/vehicle_status`
- `/fmu/out/vehicle_local_position`
- `/fmu/out/battery_status`
- `/range/front`, `/range/back`, `/range/left`, `/range/right` (optional)

### Publications (to PX4)
- `/fmu/in/vehicle_command`
- `/fmu/in/offboard_control_mode`
- `/fmu/in/trajectory_setpoint`

## Offboard Mode

The bridge implements the PX4 offboard mode correctly:
1. Streams setpoints at >= 20Hz before switching to offboard mode
2. Continues streaming setpoints while in offboard mode
3. Position or velocity control can be selected

## Usage with VR Client

1. Start the UDP bridge:
   ```bash
   ros2 launch wicom_udp udp_server.launch.py
   ```

2. Start PX4 (SITL or hardware)

3. Start px4_ros_com bridge (if needed)

4. Launch the VR client (Unity project)

5. The VR client can now:
   - Send arm/takeoff commands
   - Receive drone status updates
   - Control the drone position/velocity

## Troubleshooting

- **UDP port binding failed**: Check if port 12345 is already in use
- **No client connected**: The bridge waits for the first UDP packet from the VR client
- **Offboard mode not working**: Ensure setpoints are being streamed at >= 20Hz
- **Position control timeout**: Check that position commands are sent frequently enough

## License

MIT
