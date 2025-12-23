# WICOM-ROS2-CORE

ROS2 Humble implementation of the WICOM UDP bridge for PX4 drone control.

## Overview

This repository contains the ROS2 port of the WICOM UDP bridge, which enables control of PX4 drones via a UDP interface using the UAVLink protocol. The bridge is designed to work with the VR client from the [WICOM-VR-COPILOT](https://github.com/cinocat/WICOM-VR-COPILOT) repository.

## Features

- **UDP Server**: Receives control commands on port 12345
- **UAVLink Protocol**: Compatible with existing VR client
- **PX4 Integration**: Uses px4_ros_com and px4_msgs for drone control
- **Offboard Control**: Proper implementation of PX4 offboard mode
- **Real-time Status**: Sends drone status back to VR client at configurable rate
- **Position/Velocity Control**: Supports both position and velocity setpoints

## System Requirements

- Ubuntu 22.04 (Server or Desktop)
- ROS2 Humble
- PX4 with px4_ros_com bridge
- Network connection to VR client

## Installation

### 1. Install ROS2 Humble

Follow the official ROS2 Humble installation guide: https://docs.ros.org/en/humble/Installation.html

```bash
# Source ROS2
source /opt/ros/humble/setup.bash
```

### 2. Install PX4 and px4_ros_com

```bash
# Create workspace
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src

# Clone px4_msgs and px4_ros_com (use appropriate branch for your PX4 version)
git clone https://github.com/PX4/px4_msgs.git -b release/1.14
git clone https://github.com/PX4/px4_ros_com.git -b release/1.14

# Clone this repository
git clone https://github.com/cinocat/WICOM-ROS2-CORE.git

# Build workspace
cd ~/ros2_ws
colcon build
```

### 3. Setup Environment

```bash
# Source the workspace
source ~/ros2_ws/install/setup.bash
```

## Quick Start

### 1. Start PX4 (SITL or Hardware)

For SITL simulation:
```bash
cd ~/PX4-Autopilot
make px4_sitl gz_x500
```

### 2. Start px4_ros_com bridge

```bash
# In a new terminal
source ~/ros2_ws/install/setup.bash
ros2 run px4_ros_com micrortps_agent -t UDP
```

### 3. Start WICOM UDP Bridge

```bash
# In a new terminal
source ~/ros2_ws/install/setup.bash
ros2 launch wicom_udp udp_server.launch.py
```

### 4. Connect VR Client

Launch the Unity VR client from the WICOM-VR-COPILOT repository. It should automatically connect to port 12345.

## Configuration

Edit `wicom_udp/config/udp_server.yaml` to customize:

- **port**: UDP port (default: 12345)
- **status_send_rate**: Rate for sending drone status (default: 0.5s)
- **position_timeout**: Timeout for position commands (default: 0.1s)
- **offboard_rate**: Rate for offboard setpoints (default: 0.05s = 20Hz)

## Package Structure

```
WICOM-ROS2-CORE/
├── wicom_udp/                    # Main UDP bridge package
│   ├── include/wicom_udp/
│   │   ├── uavlink.h            # UAVLink protocol definitions
│   │   └── udp_server_node.h   # Node header
│   ├── src/
│   │   ├── udp_server_node.cpp # Node implementation
│   │   └── udp_server_main.cpp # Main executable
│   ├── launch/
│   │   └── udp_server.launch.py # Launch file
│   ├── config/
│   │   └── udp_server.yaml     # Configuration
│   ├── CMakeLists.txt
│   ├── package.xml
│   └── README.md               # Package-specific docs
├── setup_workspace.sh          # Workspace setup script
└── README.md                   # This file
```

## Architecture

```
VR Client (Unity)
    ↕ UDP (port 12345, UAVLink protocol)
WICOM UDP Bridge (wicom_udp)
    ↕ ROS2 Topics (px4_msgs)
px4_ros_com (MicroRTPS Bridge)
    ↕ RTPS/DDS
PX4 Flight Controller
```

## Supported Commands

### From VR Client to Drone
- Arm/Disarm
- Set mode (Offboard)
- Takeoff (with target altitude)
- Land
- Position control (x, y, z, yaw)
- Velocity control (vx, vy, vz, yaw_rate)

### From Drone to VR Client
- Drone status (position, velocity, orientation, battery, armed state, mode)
- Position feedback (after position commands)
- State updates

## Troubleshooting

### UDP Port Already in Use
```bash
# Check what's using port 12345
sudo netstat -tulpn | grep 12345

# Change port in config file or launch with:
ros2 launch wicom_udp udp_server.launch.py port:=12346
```

### No Client Connection
The bridge waits for the first UDP packet from the VR client before sending data. Check:
- VR client is running and configured for correct IP/port
- Firewall allows UDP traffic on port 12345
- Network connectivity between systems

### Offboard Mode Not Working
- Ensure setpoints are being published at >= 20Hz
- Check PX4 parameters (COM_RCL_EXCEPT, COM_OF_LOSS_T)
- Verify px4_ros_com bridge is running

### Build Errors
- Ensure ROS2 Humble is sourced: `source /opt/ros/humble/setup.bash`
- Install px4_msgs: `sudo apt install ros-humble-px4-msgs` or build from source
- Check all dependencies are installed

## Development

### Building in Debug Mode
```bash
colcon build --packages-select wicom_udp --cmake-args -DCMAKE_BUILD_TYPE=Debug
```

### Viewing Logs
```bash
# Terminal output
ros2 launch wicom_udp udp_server.launch.py

# ROS2 logger
ros2 topic echo /rosout
```

### Monitoring Topics
```bash
# List all topics
ros2 topic list

# View PX4 vehicle status
ros2 topic echo /fmu/out/vehicle_status

# View trajectory setpoints
ros2 topic echo /fmu/in/trajectory_setpoint
```

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

MIT License - See LICENSE file for details

## Related Repositories

- [WICOM-VR-COPILOT](https://github.com/cinocat/WICOM-VR-COPILOT) - VR client Unity project
- [WICOM-ROS1-CORE](https://github.com/cinocat/WICOM-ROS1-CORE) - Original ROS1 implementation
- [PX4-Autopilot](https://github.com/PX4/PX4-Autopilot) - PX4 flight controller
- [px4_ros_com](https://github.com/PX4/px4_ros_com) - PX4 ROS2 bridge

## Contact

For issues, questions, or contributions, please open an issue on GitHub.