#ifndef WICOM_UDP_UAVLINK_H
#define WICOM_UDP_UAVLINK_H

#include <cstdint>
#include <cstring>
#include <arpa/inet.h>

// UAVLink message IDs
#define UAVLINK_MSG_ID_STATE 1
#define UAVLINK_MSG_ID_POSITION_FEEDBACK 2
#define UAVLINK_MSG_ID_DRONE_STATUS 3
#define UAVLINK_MSG_ID_COMMAND 10
#define UAVLINK_MSG_ID_POSITION_CONTROL 11
#define UAVLINK_MSG_ID_VELOCITY_CONTROL 12

// UAVLink command IDs
#define UAVLINK_CMD_ARM_DISARM 1
#define UAVLINK_CMD_SET_MODE 2
#define UAVLINK_CMD_TAKEOFF 3
#define UAVLINK_CMD_LAND 4
#define UAVLINK_CMD_POSITION_CONTROL_MODE 5
#define UAVLINK_CMD_RC_OVERRIDE 6
#define UAVLINK_CMD_SERVO 7

// Mode definitions
#define UAVLINK_MODE_MANUAL 0
#define UAVLINK_MODE_OFFBOARD 4
#define UAVLINK_MODE_AUTO_TAKEOFF 10
#define UAVLINK_MODE_AUTO_LAND 9

// Header structure
#pragma pack(push, 1)
struct UAVLinkHeader {
    uint8_t msgid;
    uint8_t length;
};

// Command message
struct UAVLinkCommand {
    UAVLinkHeader header;
    uint8_t cmd_id;
    float param1;
    float param2;
    float param3;
    float param4;
};

// Position control message
struct UAVLinkPositionControl {
    UAVLinkHeader header;
    float x;      // meters
    float y;      // meters
    float z;      // meters
    float yaw;    // radians
};

// Velocity control message
struct UAVLinkVelocityControl {
    UAVLinkHeader header;
    float vx;     // m/s
    float vy;     // m/s
    float vz;     // m/s
    float yaw_rate; // rad/s
};

// State message (sent to VR client)
struct UAVLinkState {
    UAVLinkHeader header;
    float roll;       // radians
    float pitch;      // radians
    float yaw;        // radians
    uint8_t armed;
    uint8_t mode;
};

// Position feedback message (sent to VR client)
struct UAVLinkPositionFeedback {
    UAVLinkHeader header;
    float x;          // meters
    float y;          // meters
    float z;          // meters
    uint8_t success;
};

// Drone status message (sent to VR client)
struct UAVLinkDroneStatus {
    UAVLinkHeader header;
    float x;          // meters (local position)
    float y;          // meters
    float z;          // meters
    float vx;         // m/s
    float vy;         // m/s
    float vz;         // m/s
    float roll;       // radians
    float pitch;      // radians
    float yaw;        // radians
    float battery;    // voltage
    uint8_t armed;
    uint8_t mode;
    float range_front;  // meters
    float range_back;   // meters
    float range_left;   // meters
    float range_right;  // meters
};

#pragma pack(pop)

// Helper functions for network byte order conversion
inline void pack_float(uint8_t* buffer, float value) {
    uint32_t temp;
    memcpy(&temp, &value, sizeof(float));
    temp = htonl(temp);
    memcpy(buffer, &temp, sizeof(uint32_t));
}

inline float unpack_float(const uint8_t* buffer) {
    uint32_t temp;
    memcpy(&temp, buffer, sizeof(uint32_t));
    temp = ntohl(temp);
    float value;
    memcpy(&value, &temp, sizeof(float));
    return value;
}

inline void serialize_drone_status(uint8_t* buffer, const UAVLinkDroneStatus& status) {
    size_t offset = 0;
    buffer[offset++] = status.header.msgid;
    buffer[offset++] = status.header.length;
    
    pack_float(buffer + offset, status.x); offset += 4;
    pack_float(buffer + offset, status.y); offset += 4;
    pack_float(buffer + offset, status.z); offset += 4;
    pack_float(buffer + offset, status.vx); offset += 4;
    pack_float(buffer + offset, status.vy); offset += 4;
    pack_float(buffer + offset, status.vz); offset += 4;
    pack_float(buffer + offset, status.roll); offset += 4;
    pack_float(buffer + offset, status.pitch); offset += 4;
    pack_float(buffer + offset, status.yaw); offset += 4;
    pack_float(buffer + offset, status.battery); offset += 4;
    
    buffer[offset++] = status.armed;
    buffer[offset++] = status.mode;
    
    pack_float(buffer + offset, status.range_front); offset += 4;
    pack_float(buffer + offset, status.range_back); offset += 4;
    pack_float(buffer + offset, status.range_left); offset += 4;
    pack_float(buffer + offset, status.range_right); offset += 4;
}

inline void serialize_state(uint8_t* buffer, const UAVLinkState& state) {
    size_t offset = 0;
    buffer[offset++] = state.header.msgid;
    buffer[offset++] = state.header.length;
    
    pack_float(buffer + offset, state.roll); offset += 4;
    pack_float(buffer + offset, state.pitch); offset += 4;
    pack_float(buffer + offset, state.yaw); offset += 4;
    
    buffer[offset++] = state.armed;
    buffer[offset++] = state.mode;
}

inline void serialize_position_feedback(uint8_t* buffer, const UAVLinkPositionFeedback& feedback) {
    size_t offset = 0;
    buffer[offset++] = feedback.header.msgid;
    buffer[offset++] = feedback.header.length;
    
    pack_float(buffer + offset, feedback.x); offset += 4;
    pack_float(buffer + offset, feedback.y); offset += 4;
    pack_float(buffer + offset, feedback.z); offset += 4;
    
    buffer[offset++] = feedback.success;
}

inline UAVLinkCommand deserialize_command(const uint8_t* buffer) {
    UAVLinkCommand cmd;
    size_t offset = 0;
    
    cmd.header.msgid = buffer[offset++];
    cmd.header.length = buffer[offset++];
    cmd.cmd_id = buffer[offset++];
    
    cmd.param1 = unpack_float(buffer + offset); offset += 4;
    cmd.param2 = unpack_float(buffer + offset); offset += 4;
    cmd.param3 = unpack_float(buffer + offset); offset += 4;
    cmd.param4 = unpack_float(buffer + offset); offset += 4;
    
    return cmd;
}

inline UAVLinkPositionControl deserialize_position_control(const uint8_t* buffer) {
    UAVLinkPositionControl pos;
    size_t offset = 0;
    
    pos.header.msgid = buffer[offset++];
    pos.header.length = buffer[offset++];
    
    pos.x = unpack_float(buffer + offset); offset += 4;
    pos.y = unpack_float(buffer + offset); offset += 4;
    pos.z = unpack_float(buffer + offset); offset += 4;
    pos.yaw = unpack_float(buffer + offset); offset += 4;
    
    return pos;
}

inline UAVLinkVelocityControl deserialize_velocity_control(const uint8_t* buffer) {
    UAVLinkVelocityControl vel;
    size_t offset = 0;
    
    vel.header.msgid = buffer[offset++];
    vel.header.length = buffer[offset++];
    
    vel.vx = unpack_float(buffer + offset); offset += 4;
    vel.vy = unpack_float(buffer + offset); offset += 4;
    vel.vz = unpack_float(buffer + offset); offset += 4;
    vel.yaw_rate = unpack_float(buffer + offset); offset += 4;
    
    return vel;
}

#endif // WICOM_UDP_UAVLINK_H
