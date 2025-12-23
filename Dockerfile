# Dockerfile for WICOM ROS2 Development and Testing
FROM osrf/ros:humble-desktop

# Install dependencies
RUN apt-get update && apt-get install -y \
    git \
    python3-pip \
    python3-colcon-common-extensions \
    ros-humble-sensor-msgs \
    ros-humble-geometry-msgs \
    && rm -rf /var/lib/apt/lists/*

# Create workspace
RUN mkdir -p /ros2_ws/src
WORKDIR /ros2_ws

# Clone px4_msgs (this would normally be built separately)
# Note: Users need to clone this or install it
# RUN cd src && git clone https://github.com/PX4/px4_msgs.git -b release/1.14

# Copy the wicom_udp package
COPY wicom_udp /ros2_ws/src/wicom_udp

# Source ROS2 and build (will fail without px4_msgs, but shows the process)
RUN /bin/bash -c "source /opt/ros/humble/setup.bash && \
    echo 'Note: This build will fail without px4_msgs. Install it first.'"

# Setup entrypoint
COPY docker-entrypoint.sh /
RUN chmod +x /docker-entrypoint.sh
ENTRYPOINT ["/docker-entrypoint.sh"]

CMD ["bash"]
