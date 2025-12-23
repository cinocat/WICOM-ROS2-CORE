#!/bin/bash
# Setup script for WICOM ROS2 workspace

set -e

echo "=== WICOM ROS2 Workspace Setup ==="

# Check if ROS2 Humble is sourced
if [ -z "$ROS_DISTRO" ]; then
    echo "Error: ROS2 not sourced. Please source ROS2 Humble first:"
    echo "  source /opt/ros/humble/setup.bash"
    exit 1
fi

if [ "$ROS_DISTRO" != "humble" ]; then
    echo "Warning: Expected ROS2 Humble, but found $ROS_DISTRO"
fi

echo "ROS2 Distribution: $ROS_DISTRO"

# Check for required dependencies
echo ""
echo "Checking dependencies..."

# Check for px4_msgs
if ! ros2 pkg list | grep -q "px4_msgs"; then
    echo "Warning: px4_msgs not found. You need to install px4_ros_com:"
    echo "  git clone https://github.com/PX4/px4_ros_com.git -b release/1.14"
    echo "  git clone https://github.com/PX4/px4_msgs.git -b release/1.14"
    echo "  colcon build --packages-select px4_msgs"
fi

# Create workspace if not exists
WORKSPACE_DIR=$(pwd)
echo ""
echo "Workspace directory: $WORKSPACE_DIR"

# Build the package
echo ""
echo "Building wicom_udp package..."
colcon build --packages-select wicom_udp --cmake-args -DCMAKE_BUILD_TYPE=Release

echo ""
echo "=== Build Complete ==="
echo ""
echo "To use the package, source the workspace:"
echo "  source $WORKSPACE_DIR/install/setup.bash"
echo ""
echo "Then run the node:"
echo "  ros2 launch wicom_udp udp_server.launch.py"
