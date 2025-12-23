#include "wicom_udp/udp_server_node.h"
#include <unistd.h>
#include <cmath>

UdpServerNode::UdpServerNode() : Node("udp_server_node"), 
    udp_socket_(-1),
    client_connected_(false),
    running_(true),
    range_front_(0.0),
    range_back_(0.0),
    range_left_(0.0),
    range_right_(0.0),
    position_control_active_(false),
    velocity_control_active_(false),
    offboard_mode_active_(false)
{
    // Declare and get parameters
    this->declare_parameter<int>("port", 12345);
    this->declare_parameter<double>("status_send_rate", 0.5);
    this->declare_parameter<double>("position_timeout", 0.1);
    this->declare_parameter<double>("offboard_rate", 0.05); // 20Hz
    
    this->get_parameter("port", udp_port_);
    this->get_parameter("status_send_rate", status_send_rate_);
    this->get_parameter("position_timeout", position_timeout_);
    this->get_parameter("offboard_rate", offboard_rate_);
    
    RCLCPP_INFO(this->get_logger(), "Starting UDP server on port %d", udp_port_);
    
    // Setup UDP socket
    udp_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket_ < 0) {
        RCLCPP_ERROR(this->get_logger(), "Failed to create UDP socket");
        throw std::runtime_error("Failed to create UDP socket");
    }
    
    // Set socket options
    int reuse = 1;
    if (setsockopt(udp_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        RCLCPP_WARN(this->get_logger(), "Failed to set SO_REUSEADDR");
    }
    
    // Bind socket
    memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY;
    server_addr_.sin_port = htons(udp_port_);
    
    if (bind(udp_socket_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        RCLCPP_ERROR(this->get_logger(), "Failed to bind UDP socket to port %d", udp_port_);
        close(udp_socket_);
        throw std::runtime_error("Failed to bind UDP socket");
    }
    
    RCLCPP_INFO(this->get_logger(), "UDP socket bound to port %d", udp_port_);
    
    // Initialize client address
    memset(&client_addr_, 0, sizeof(client_addr_));
    client_addr_len_ = sizeof(client_addr_);
    
    // Setup ROS2 subscribers
    vehicle_status_sub_ = this->create_subscription<px4_msgs::msg::VehicleStatus>(
        "/fmu/out/vehicle_status", 10,
        std::bind(&UdpServerNode::vehicle_status_callback, this, std::placeholders::_1));
    
    vehicle_local_position_sub_ = this->create_subscription<px4_msgs::msg::VehicleLocalPosition>(
        "/fmu/out/vehicle_local_position", 10,
        std::bind(&UdpServerNode::vehicle_local_position_callback, this, std::placeholders::_1));
    
    battery_status_sub_ = this->create_subscription<px4_msgs::msg::BatteryStatus>(
        "/fmu/out/battery_status", 10,
        std::bind(&UdpServerNode::battery_status_callback, this, std::placeholders::_1));
    
    // Range sensors (optional)
    range_front_sub_ = this->create_subscription<sensor_msgs::msg::Range>(
        "/range/front", 10,
        [this](const sensor_msgs::msg::Range::SharedPtr msg) { range_callback(msg, 0); });
    
    range_back_sub_ = this->create_subscription<sensor_msgs::msg::Range>(
        "/range/back", 10,
        [this](const sensor_msgs::msg::Range::SharedPtr msg) { range_callback(msg, 1); });
    
    range_left_sub_ = this->create_subscription<sensor_msgs::msg::Range>(
        "/range/left", 10,
        [this](const sensor_msgs::msg::Range::SharedPtr msg) { range_callback(msg, 2); });
    
    range_right_sub_ = this->create_subscription<sensor_msgs::msg::Range>(
        "/range/right", 10,
        [this](const sensor_msgs::msg::Range::SharedPtr msg) { range_callback(msg, 3); });
    
    // Setup ROS2 publishers
    vehicle_command_pub_ = this->create_publisher<px4_msgs::msg::VehicleCommand>(
        "/fmu/in/vehicle_command", 10);
    
    offboard_control_mode_pub_ = this->create_publisher<px4_msgs::msg::OffboardControlMode>(
        "/fmu/in/offboard_control_mode", 10);
    
    trajectory_setpoint_pub_ = this->create_publisher<px4_msgs::msg::TrajectorySetpoint>(
        "/fmu/in/trajectory_setpoint", 10);
    
    // Setup timers
    drone_status_timer_ = this->create_wall_timer(
        std::chrono::duration<double>(status_send_rate_),
        std::bind(&UdpServerNode::send_drone_status_timer_callback, this));
    
    position_timeout_timer_ = this->create_wall_timer(
        std::chrono::duration<double>(position_timeout_),
        std::bind(&UdpServerNode::position_timeout_watchdog_callback, this));
    
    offboard_setpoint_timer_ = this->create_wall_timer(
        std::chrono::duration<double>(offboard_rate_),
        std::bind(&UdpServerNode::offboard_setpoint_timer_callback, this));
    
    // Initialize setpoint
    current_setpoint_.position = {NAN, NAN, NAN};
    current_setpoint_.velocity = {NAN, NAN, NAN};
    current_setpoint_.acceleration = {NAN, NAN, NAN};
    current_setpoint_.jerk = {NAN, NAN, NAN};
    current_setpoint_.yaw = NAN;
    current_setpoint_.yawspeed = NAN;
    
    // Start UDP receive thread
    udp_thread_ = std::thread(&UdpServerNode::udp_receive_thread, this);
    
    RCLCPP_INFO(this->get_logger(), "UDP Server Node initialized");
}

UdpServerNode::~UdpServerNode() {
    running_ = false;
    
    if (udp_thread_.joinable()) {
        udp_thread_.join();
    }
    
    if (udp_socket_ >= 0) {
        close(udp_socket_);
    }
}

uint64_t UdpServerNode::timestamp_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void UdpServerNode::udp_receive_thread() {
    uint8_t buffer[1024];
    
    while (running_) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(udp_socket_, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms
        
        int ret = select(udp_socket_ + 1, &read_fds, NULL, NULL, &timeout);
        
        if (ret > 0 && FD_ISSET(udp_socket_, &read_fds)) {
            sockaddr_in recv_addr;
            socklen_t recv_addr_len = sizeof(recv_addr);
            
            ssize_t received = recvfrom(udp_socket_, buffer, sizeof(buffer), 0,
                                       (struct sockaddr*)&recv_addr, &recv_addr_len);
            
            if (received > 0) {
                // Store client address on first packet
                if (!client_connected_) {
                    std::lock_guard<std::mutex> lock(client_addr_mutex_);
                    client_addr_ = recv_addr;
                    client_addr_len_ = recv_addr_len;
                    client_connected_ = true;
                    RCLCPP_INFO(this->get_logger(), "Client connected from %s:%d",
                               inet_ntoa(recv_addr.sin_addr), ntohs(recv_addr.sin_port));
                }
                
                process_udp_packet(buffer, received);
            }
        }
    }
}

void UdpServerNode::process_udp_packet(const uint8_t* buffer, size_t length) {
    if (length < 2) {
        RCLCPP_WARN(this->get_logger(), "Received packet too short");
        return;
    }
    
    uint8_t msgid = buffer[0];
    
    switch (msgid) {
        case UAVLINK_MSG_ID_COMMAND: {
            if (length >= sizeof(UAVLinkCommand)) {
                UAVLinkCommand cmd = deserialize_command(buffer);
                handle_command(cmd);
            }
            break;
        }
        case UAVLINK_MSG_ID_POSITION_CONTROL: {
            if (length >= sizeof(UAVLinkPositionControl)) {
                UAVLinkPositionControl pos = deserialize_position_control(buffer);
                handle_position_control(pos);
            }
            break;
        }
        case UAVLINK_MSG_ID_VELOCITY_CONTROL: {
            if (length >= sizeof(UAVLinkVelocityControl)) {
                UAVLinkVelocityControl vel = deserialize_velocity_control(buffer);
                handle_velocity_control(vel);
            }
            break;
        }
        default:
            RCLCPP_DEBUG(this->get_logger(), "Unknown message ID: %d", msgid);
            break;
    }
}

void UdpServerNode::send_udp_packet(const uint8_t* buffer, size_t length) {
    if (!client_connected_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(client_addr_mutex_);
    sendto(udp_socket_, buffer, length, 0,
           (struct sockaddr*)&client_addr_, client_addr_len_);
}

void UdpServerNode::handle_command(const UAVLinkCommand& cmd) {
    RCLCPP_INFO(this->get_logger(), "Received command: %d", cmd.cmd_id);
    
    switch (cmd.cmd_id) {
        case UAVLINK_CMD_ARM_DISARM:
            if (cmd.param1 > 0.5) {
                arm();
            } else {
                disarm();
            }
            break;
            
        case UAVLINK_CMD_SET_MODE:
            if (static_cast<int>(cmd.param1) == UAVLINK_MODE_OFFBOARD) {
                set_mode_offboard();
            }
            break;
            
        case UAVLINK_CMD_TAKEOFF:
            takeoff(cmd.param1);
            break;
            
        case UAVLINK_CMD_LAND:
            land();
            break;
            
        case UAVLINK_CMD_POSITION_CONTROL_MODE:
            position_control_active_ = (cmd.param1 > 0.5);
            if (!position_control_active_) {
                // Send position feedback failure
                UAVLinkPositionFeedback feedback;
                feedback.header.msgid = UAVLINK_MSG_ID_POSITION_FEEDBACK;
                feedback.header.length = sizeof(UAVLinkPositionFeedback) - sizeof(UAVLinkHeader);
                feedback.x = 0;
                feedback.y = 0;
                feedback.z = 0;
                feedback.success = 0;
                
                uint8_t buffer[64];
                serialize_position_feedback(buffer, feedback);
                send_udp_packet(buffer, sizeof(UAVLinkPositionFeedback));
            }
            break;
            
        default:
            RCLCPP_WARN(this->get_logger(), "Unhandled command: %d", cmd.cmd_id);
            break;
    }
}

void UdpServerNode::handle_position_control(const UAVLinkPositionControl& pos) {
    RCLCPP_DEBUG(this->get_logger(), "Position control: x=%.2f, y=%.2f, z=%.2f, yaw=%.2f",
                 pos.x, pos.y, pos.z, pos.yaw);
    
    if (position_control_active_) {
        current_setpoint_.position[0] = pos.x;
        current_setpoint_.position[1] = pos.y;
        current_setpoint_.position[2] = -pos.z; // NED frame: down is positive
        current_setpoint_.yaw = pos.yaw;
        
        // Clear velocity/acceleration
        current_setpoint_.velocity = {NAN, NAN, NAN};
        current_setpoint_.acceleration = {NAN, NAN, NAN};
        current_setpoint_.yawspeed = NAN;
        
        last_position_command_time_ = this->now();
        
        // Send position feedback success
        UAVLinkPositionFeedback feedback;
        feedback.header.msgid = UAVLINK_MSG_ID_POSITION_FEEDBACK;
        feedback.header.length = sizeof(UAVLinkPositionFeedback) - sizeof(UAVLinkHeader);
        feedback.x = pos.x;
        feedback.y = pos.y;
        feedback.z = pos.z;
        feedback.success = 1;
        
        uint8_t buffer[64];
        serialize_position_feedback(buffer, feedback);
        send_udp_packet(buffer, sizeof(UAVLinkPositionFeedback));
    }
}

void UdpServerNode::handle_velocity_control(const UAVLinkVelocityControl& vel) {
    RCLCPP_DEBUG(this->get_logger(), "Velocity control: vx=%.2f, vy=%.2f, vz=%.2f",
                 vel.vx, vel.vy, vel.vz);
    
    if (velocity_control_active_) {
        current_setpoint_.velocity[0] = vel.vx;
        current_setpoint_.velocity[1] = vel.vy;
        current_setpoint_.velocity[2] = -vel.vz; // NED frame
        current_setpoint_.yawspeed = vel.yaw_rate;
        
        // Clear position/acceleration
        current_setpoint_.position = {NAN, NAN, NAN};
        current_setpoint_.acceleration = {NAN, NAN, NAN};
        current_setpoint_.yaw = NAN;
        
        last_velocity_command_time_ = this->now();
    }
}

void UdpServerNode::arm() {
    RCLCPP_INFO(this->get_logger(), "Arming...");
    send_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM, 1.0);
}

void UdpServerNode::disarm() {
    RCLCPP_INFO(this->get_logger(), "Disarming...");
    send_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM, 0.0);
}

void UdpServerNode::set_mode_offboard() {
    RCLCPP_INFO(this->get_logger(), "Setting OFFBOARD mode...");
    send_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_DO_SET_MODE, 1.0, 6.0);
    offboard_mode_active_ = true;
}

void UdpServerNode::takeoff(float altitude) {
    RCLCPP_INFO(this->get_logger(), "Takeoff to altitude: %.2f", altitude);
    
    // Set initial position setpoint at current position but target altitude
    if (current_local_position_) {
        current_setpoint_.position[0] = current_local_position_->x;
        current_setpoint_.position[1] = current_local_position_->y;
        current_setpoint_.position[2] = -altitude; // NED frame
        current_setpoint_.yaw = current_local_position_->heading;
        
        current_setpoint_.velocity = {NAN, NAN, NAN};
        current_setpoint_.acceleration = {NAN, NAN, NAN};
        current_setpoint_.yawspeed = NAN;
        
        position_control_active_ = true;
        last_position_command_time_ = this->now();
        
        // Enable offboard mode after a brief period
        offboard_mode_active_ = true;
    }
    
    // Alternative: use PX4 takeoff command
    send_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_TAKEOFF, 
                        0.0, 0.0, 0.0, NAN, 0.0, 0.0, altitude);
}

void UdpServerNode::land() {
    RCLCPP_INFO(this->get_logger(), "Landing...");
    send_vehicle_command(px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_LAND);
    position_control_active_ = false;
    velocity_control_active_ = false;
}

void UdpServerNode::send_vehicle_command(uint16_t command, float param1,
                                         float param2, float param3, float param4,
                                         float param5, float param6, float param7) {
    px4_msgs::msg::VehicleCommand msg;
    msg.timestamp = timestamp_us();
    msg.param1 = param1;
    msg.param2 = param2;
    msg.param3 = param3;
    msg.param4 = param4;
    msg.param5 = param5;
    msg.param6 = param6;
    msg.param7 = param7;
    msg.command = command;
    msg.target_system = 1;
    msg.target_component = 1;
    msg.source_system = 1;
    msg.source_component = 1;
    msg.from_external = true;
    
    vehicle_command_pub_->publish(msg);
}

void UdpServerNode::send_drone_status_timer_callback() {
    if (!client_connected_) {
        return;
    }
    
    UAVLinkDroneStatus status;
    status.header.msgid = UAVLINK_MSG_ID_DRONE_STATUS;
    status.header.length = sizeof(UAVLinkDroneStatus) - sizeof(UAVLinkHeader);
    
    // Fill in position and velocity
    if (current_local_position_) {
        status.x = current_local_position_->x;
        status.y = current_local_position_->y;
        status.z = -current_local_position_->z; // Convert from NED
        status.vx = current_local_position_->vx;
        status.vy = current_local_position_->vy;
        status.vz = -current_local_position_->vz;
        
        // Convert quaternion to Euler angles (simplified - yaw only from heading)
        status.roll = 0.0; // Would need full quaternion conversion
        status.pitch = 0.0;
        status.yaw = current_local_position_->heading;
    } else {
        status.x = status.y = status.z = 0.0;
        status.vx = status.vy = status.vz = 0.0;
        status.roll = status.pitch = status.yaw = 0.0;
    }
    
    // Fill in battery
    if (current_battery_status_) {
        status.battery = current_battery_status_->voltage_v;
    } else {
        status.battery = 0.0;
    }
    
    // Fill in armed and mode
    if (current_vehicle_status_) {
        status.armed = current_vehicle_status_->arming_state == 
                       px4_msgs::msg::VehicleStatus::ARMING_STATE_ARMED ? 1 : 0;
        status.mode = current_vehicle_status_->nav_state;
    } else {
        status.armed = 0;
        status.mode = 0;
    }
    
    // Fill in range sensors
    status.range_front = range_front_;
    status.range_back = range_back_;
    status.range_left = range_left_;
    status.range_right = range_right_;
    
    // Serialize and send
    uint8_t buffer[128];
    serialize_drone_status(buffer, status);
    send_udp_packet(buffer, sizeof(UAVLinkDroneStatus));
    
    RCLCPP_DEBUG(this->get_logger(), "Sent drone status");
}

void UdpServerNode::position_timeout_watchdog_callback() {
    if (position_control_active_) {
        auto elapsed = (this->now() - last_position_command_time_).seconds();
        if (elapsed > position_timeout_) {
            RCLCPP_WARN(this->get_logger(), "Position command timeout (%.2fs)", elapsed);
            position_control_active_ = false;
            
            // Send position feedback failure
            UAVLinkPositionFeedback feedback;
            feedback.header.msgid = UAVLINK_MSG_ID_POSITION_FEEDBACK;
            feedback.header.length = sizeof(UAVLinkPositionFeedback) - sizeof(UAVLinkHeader);
            feedback.x = 0;
            feedback.y = 0;
            feedback.z = 0;
            feedback.success = 0;
            
            uint8_t buffer[64];
            serialize_position_feedback(buffer, feedback);
            send_udp_packet(buffer, sizeof(UAVLinkPositionFeedback));
        }
    }
}

void UdpServerNode::offboard_setpoint_timer_callback() {
    if (offboard_mode_active_ && (position_control_active_ || velocity_control_active_)) {
        // Publish offboard control mode
        px4_msgs::msg::OffboardControlMode control_mode;
        control_mode.timestamp = timestamp_us();
        control_mode.position = position_control_active_;
        control_mode.velocity = velocity_control_active_;
        control_mode.acceleration = false;
        control_mode.attitude = false;
        control_mode.body_rate = false;
        
        offboard_control_mode_pub_->publish(control_mode);
        
        // Publish trajectory setpoint
        current_setpoint_.timestamp = timestamp_us();
        trajectory_setpoint_pub_->publish(current_setpoint_);
        
        RCLCPP_DEBUG(this->get_logger(), "Published offboard setpoint");
    }
}

void UdpServerNode::vehicle_status_callback(const px4_msgs::msg::VehicleStatus::SharedPtr msg) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    current_vehicle_status_ = msg;
}

void UdpServerNode::vehicle_local_position_callback(
    const px4_msgs::msg::VehicleLocalPosition::SharedPtr msg) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    current_local_position_ = msg;
}

void UdpServerNode::battery_status_callback(const px4_msgs::msg::BatteryStatus::SharedPtr msg) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    current_battery_status_ = msg;
}

void UdpServerNode::range_callback(const sensor_msgs::msg::Range::SharedPtr msg, int sensor_id) {
    switch (sensor_id) {
        case 0: range_front_ = msg->range; break;
        case 1: range_back_ = msg->range; break;
        case 2: range_left_ = msg->range; break;
        case 3: range_right_ = msg->range; break;
    }
}
