# UAVLink Protocol Quick Reference

This document provides a quick reference for the UAVLink protocol used between the VR client and the ROS2 UDP bridge.

## Message Structure

All messages have a 2-byte header:
- Byte 0: Message ID
- Byte 1: Payload length (bytes following the header)

Floats are encoded in network byte order (big-endian).

## Message IDs

### Messages from VR Client to Bridge

| ID | Name | Description |
|----|------|-------------|
| 10 | COMMAND | General command (arm, mode, takeoff, land, etc.) |
| 11 | POSITION_CONTROL | Position setpoint (x, y, z, yaw) |
| 12 | VELOCITY_CONTROL | Velocity setpoint (vx, vy, vz, yaw_rate) |

### Messages from Bridge to VR Client

| ID | Name | Description |
|----|------|-------------|
| 1 | STATE | Vehicle orientation and status |
| 2 | POSITION_FEEDBACK | Position command acknowledgment |
| 3 | DRONE_STATUS | Complete drone state |

## Command Message (ID: 10)

**Structure:**
```
Byte 0-1: Header (msgid=10, length=19)
Byte 2: Command ID
Bytes 3-6: param1 (float, big-endian)
Bytes 7-10: param2 (float, big-endian)
Bytes 11-14: param3 (float, big-endian)
Bytes 15-18: param4 (float, big-endian)
Total: 21 bytes
```

**Command IDs:**

| ID | Name | param1 | param2 | param3 | param4 |
|----|------|--------|--------|--------|--------|
| 1 | ARM_DISARM | 1.0=arm, 0.0=disarm | - | - | - |
| 2 | SET_MODE | mode (4=OFFBOARD) | - | - | - |
| 3 | TAKEOFF | altitude (meters) | - | - | - |
| 4 | LAND | - | - | - | - |
| 5 | POSITION_CONTROL_MODE | 1.0=enable, 0.0=disable | - | - | - |
| 8 | VELOCITY_CONTROL_MODE | 1.0=enable, 0.0=disable | - | - | - |

## Position Control Message (ID: 11)

**Structure:**
```
Byte 0-1: Header (msgid=11, length=16)
Bytes 2-5: x (float, meters, big-endian)
Bytes 6-9: y (float, meters, big-endian)
Bytes 10-13: z (float, meters, big-endian)
Bytes 14-17: yaw (float, radians, big-endian)
Total: 18 bytes
```

**Coordinate Frame:**
- x: East (positive = move east)
- y: North (positive = move north)
- z: Up (positive = move up)
- yaw: Heading (0 = north, π/2 = east, π = south, -π/2 = west)

## Velocity Control Message (ID: 12)

**Structure:**
```
Byte 0-1: Header (msgid=12, length=16)
Bytes 2-5: vx (float, m/s, big-endian)
Bytes 6-9: vy (float, m/s, big-endian)
Bytes 10-13: vz (float, m/s, big-endian)
Bytes 14-17: yaw_rate (float, rad/s, big-endian)
Total: 18 bytes
```

## State Message (ID: 1)

**Structure (sent by bridge):**
```
Byte 0-1: Header (msgid=1, length=14)
Bytes 2-5: roll (float, radians, big-endian)
Bytes 6-9: pitch (float, radians, big-endian)
Bytes 10-13: yaw (float, radians, big-endian)
Byte 14: armed (1=armed, 0=disarmed)
Byte 15: mode (PX4 nav_state)
Total: 16 bytes
```

## Position Feedback Message (ID: 2)

**Structure (sent by bridge):**
```
Byte 0-1: Header (msgid=2, length=13)
Bytes 2-5: x (float, meters, big-endian)
Bytes 6-9: y (float, meters, big-endian)
Bytes 10-13: z (float, meters, big-endian)
Byte 14: success (1=success, 0=failure)
Total: 15 bytes
```

Sent in response to:
- Position control commands (immediate feedback)
- Position control mode disable (failure feedback)
- Position timeout (failure feedback)

## Drone Status Message (ID: 3)

**Structure (sent by bridge periodically):**
```
Byte 0-1: Header (msgid=3, length=58)
Bytes 2-5: x (float, meters, big-endian)
Bytes 6-9: y (float, meters, big-endian)
Bytes 10-13: z (float, meters, big-endian)
Bytes 14-17: vx (float, m/s, big-endian)
Bytes 18-21: vy (float, m/s, big-endian)
Bytes 22-25: vz (float, m/s, big-endian)
Bytes 26-29: roll (float, radians, big-endian)
Bytes 30-33: pitch (float, radians, big-endian)
Bytes 34-37: yaw (float, radians, big-endian)
Bytes 38-41: battery (float, volts, big-endian)
Byte 42: armed (1=armed, 0=disarmed)
Byte 43: mode (PX4 nav_state)
Bytes 44-47: range_front (float, meters, big-endian)
Bytes 48-51: range_back (float, meters, big-endian)
Bytes 52-55: range_left (float, meters, big-endian)
Bytes 56-59: range_right (float, meters, big-endian)
Total: 60 bytes
```

**Sending Rate:** Configurable, default 0.5s (2 Hz)

## Typical Command Sequence

### Takeoff and Offboard Control

```
1. VR → Bridge: COMMAND(TAKEOFF, altitude=5.0)
   - Bridge starts streaming setpoints at 20Hz
   
2. VR → Bridge: COMMAND(ARM_DISARM, param1=1.0)
   - Drone arms
   
3. VR → Bridge: COMMAND(SET_MODE, param1=4.0)
   - Switches to OFFBOARD mode
   - Drone begins following setpoints
   
4. VR → Bridge: COMMAND(POSITION_CONTROL_MODE, param1=1.0)
   - Enables position control
   
5. VR → Bridge: POSITION_CONTROL(x, y, z, yaw)
   - Drone moves to target position
   Bridge → VR: POSITION_FEEDBACK(x, y, z, success=1)
   
6. (Periodic) Bridge → VR: DRONE_STATUS(...)
   - VR receives continuous status updates
```

### Landing

```
1. VR → Bridge: COMMAND(LAND)
   - Drone begins landing sequence
```

## Network Byte Order (Big-Endian)

To encode a float as big-endian:

```c++
uint32_t temp;
memcpy(&temp, &float_value, sizeof(float));
temp = htonl(temp);  // Convert to network byte order
memcpy(buffer, &temp, sizeof(uint32_t));
```

To decode a float from big-endian:

```c++
uint32_t temp;
memcpy(&temp, buffer, sizeof(uint32_t));
temp = ntohl(temp);  // Convert from network byte order
float float_value;
memcpy(&float_value, &temp, sizeof(float));
```

## Unity C# Example

```csharp
// Send command to arm
byte[] buffer = new byte[21];
buffer[0] = 10;  // COMMAND message ID
buffer[1] = 19;  // Length
buffer[2] = 1;   // ARM_DISARM command
WriteFloat(buffer, 3, 1.0f);  // param1 = 1.0 (arm)
WriteFloat(buffer, 7, 0.0f);  // param2
WriteFloat(buffer, 11, 0.0f); // param3
WriteFloat(buffer, 15, 0.0f); // param4
udpClient.Send(buffer, buffer.Length);

// Helper to write float in network byte order
void WriteFloat(byte[] buffer, int offset, float value)
{
    byte[] bytes = BitConverter.GetBytes(value);
    if (BitConverter.IsLittleEndian)
        Array.Reverse(bytes);
    Array.Copy(bytes, 0, buffer, offset, 4);
}

// Helper to read float from network byte order
float ReadFloat(byte[] buffer, int offset)
{
    byte[] bytes = new byte[4];
    Array.Copy(buffer, offset, bytes, 0, 4);
    if (BitConverter.IsLittleEndian)
        Array.Reverse(bytes);
    return BitConverter.ToSingle(bytes, 0);
}
```

## Troubleshooting

**No response from bridge:**
- Check that UDP packets are being sent to port 12345
- Bridge only sends data after receiving first packet (automatic client detection)
- Check firewall settings

**Position control not working:**
- Ensure POSITION_CONTROL_MODE is enabled first
- Position timeout is 0.1s by default - send commands frequently
- Position feedback will indicate failure if timeout occurs

**Offboard mode rejected:**
- Ensure arm command is sent first
- Bridge must be streaming setpoints (send TAKEOFF or enable position control)
- Check PX4 parameters (COM_RCL_EXCEPT, COM_OF_LOSS_T)

**Coordinate system issues:**
- Bridge expects NED frame internally but presents ENU to VR client
- Z-axis: positive = up in VR messages, converted to NED internally
- Yaw: 0 = north, increases clockwise when viewed from above

## Support

For issues or questions:
- Repository: https://github.com/cinocat/WICOM-ROS2-CORE
- VR Client: https://github.com/cinocat/WICOM-VR-COPILOT
