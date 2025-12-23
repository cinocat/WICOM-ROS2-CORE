#ifndef WICOM_UDP_UDP_SERVER_NODE_H
#define WICOM_UDP_UDP_SERVER_NODE_H

#include <rclcpp/rclcpp.hpp>
#include <px4_msgs/msg/vehicle_command.hpp>
#include <px4_msgs/msg/vehicle_control_mode.hpp>
#include <px4_msgs/msg/offboard_control_mode.hpp>
#include <px4_msgs/msg/trajectory_setpoint.hpp>
#include <px4_msgs/msg/vehicle_status.hpp>
#include <px4_msgs/msg/vehicle_local_position.hpp>
#include <px4_msgs/msg/battery_status.hpp>
#include <sensor_msgs/msg/range.hpp>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>
#include <mutex>

#include "wicom_udp/uavlink.h"

class UdpServerNode : public rclcpp::Node {
public:
    UdpServerNode();
    ~UdpServerNode();

private:
    // UDP server functions
    void udp_receive_thread();
    void process_udp_packet(const uint8_t* buffer, size_t length);
    void send_udp_packet(const uint8_t* buffer, size_t length);
    
    // Message handlers
    void handle_command(const UAVLinkCommand& cmd);
    void handle_position_control(const UAVLinkPositionControl& pos);
    void handle_velocity_control(const UAVLinkVelocityControl& vel);
    
    // PX4 command functions
    void arm();
    void disarm();
    void set_mode_offboard();
    void takeoff(float altitude);
    void land();
    void send_vehicle_command(uint16_t command, float param1 = 0.0, 
                              float param2 = 0.0, float param3 = 0.0,
                              float param4 = 0.0, float param5 = 0.0,
                              float param6 = 0.0, float param7 = 0.0);
    
    // Timer callbacks
    void send_drone_status_timer_callback();
    void position_timeout_watchdog_callback();
    void offboard_setpoint_timer_callback();
    
    // ROS2 subscribers
    rclcpp::Subscription<px4_msgs::msg::VehicleStatus>::SharedPtr vehicle_status_sub_;
    rclcpp::Subscription<px4_msgs::msg::VehicleLocalPosition>::SharedPtr vehicle_local_position_sub_;
    rclcpp::Subscription<px4_msgs::msg::BatteryStatus>::SharedPtr battery_status_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Range>::SharedPtr range_front_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Range>::SharedPtr range_back_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Range>::SharedPtr range_left_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Range>::SharedPtr range_right_sub_;
    
    // ROS2 publishers
    rclcpp::Publisher<px4_msgs::msg::VehicleCommand>::SharedPtr vehicle_command_pub_;
    rclcpp::Publisher<px4_msgs::msg::OffboardControlMode>::SharedPtr offboard_control_mode_pub_;
    rclcpp::Publisher<px4_msgs::msg::TrajectorySetpoint>::SharedPtr trajectory_setpoint_pub_;
    
    // Timers
    rclcpp::TimerBase::SharedPtr drone_status_timer_;
    rclcpp::TimerBase::SharedPtr position_timeout_timer_;
    rclcpp::TimerBase::SharedPtr offboard_setpoint_timer_;
    
    // Subscriber callbacks
    void vehicle_status_callback(const px4_msgs::msg::VehicleStatus::SharedPtr msg);
    void vehicle_local_position_callback(const px4_msgs::msg::VehicleLocalPosition::SharedPtr msg);
    void battery_status_callback(const px4_msgs::msg::BatteryStatus::SharedPtr msg);
    void range_callback(const sensor_msgs::msg::Range::SharedPtr msg, int sensor_id);
    
    // UDP socket
    int udp_socket_;
    struct sockaddr_in server_addr_;
    struct sockaddr_in client_addr_;
    socklen_t client_addr_len_;
    bool client_connected_;
    std::thread udp_thread_;
    std::atomic<bool> running_;
    std::mutex client_addr_mutex_;
    
    // State variables
    std::mutex state_mutex_;
    px4_msgs::msg::VehicleStatus::SharedPtr current_vehicle_status_;
    px4_msgs::msg::VehicleLocalPosition::SharedPtr current_local_position_;
    px4_msgs::msg::BatteryStatus::SharedPtr current_battery_status_;
    
    float range_front_;
    float range_back_;
    float range_left_;
    float range_right_;
    
    // Position/velocity control state
    bool position_control_active_;
    bool velocity_control_active_;
    bool offboard_mode_active_;
    rclcpp::Time last_position_command_time_;
    rclcpp::Time last_velocity_command_time_;
    
    px4_msgs::msg::TrajectorySetpoint current_setpoint_;
    
    // Parameters
    int udp_port_;
    double status_send_rate_;
    double position_timeout_;
    double offboard_rate_;
    
    uint64_t timestamp_us();
};

#endif // WICOM_UDP_UDP_SERVER_NODE_H
