#!/usr/bin/env python3
"""
Test script to validate WICOM UDP bridge package structure and UAVLink protocol.
This script checks that all required files exist and have the correct structure.
"""

import os
import sys
import xml.etree.ElementTree as ET

def check_file_exists(filepath, description):
    """Check if a file exists."""
    if os.path.exists(filepath):
        print(f"✓ {description}: {filepath}")
        return True
    else:
        print(f"✗ {description} MISSING: {filepath}")
        return False

def check_package_xml(filepath):
    """Validate package.xml structure."""
    try:
        tree = ET.parse(filepath)
        root = tree.getroot()
        
        # Check required fields
        name = root.find('name')
        if name is not None and name.text == 'wicom_udp':
            print(f"  ✓ Package name: {name.text}")
        else:
            print(f"  ✗ Package name incorrect or missing")
            return False
            
        # Check for required dependencies
        required_deps = ['rclcpp', 'px4_msgs', 'std_msgs', 'geometry_msgs', 'sensor_msgs']
        found_deps = []
        
        for dep in root.findall('.//depend'):
            if dep.text in required_deps:
                found_deps.append(dep.text)
        
        for dep in required_deps:
            if dep in found_deps:
                print(f"  ✓ Dependency: {dep}")
            else:
                print(f"  ✗ Missing dependency: {dep}")
                return False
        
        return True
    except Exception as e:
        print(f"  ✗ Error parsing package.xml: {e}")
        return False

def check_cmake_lists(filepath):
    """Check CMakeLists.txt for required components."""
    try:
        with open(filepath, 'r') as f:
            content = f.read()
        
        required_items = [
            'project(wicom_udp)',
            'find_package(rclcpp REQUIRED)',
            'find_package(px4_msgs REQUIRED)',
            'add_executable(udp_server_node',
            'ament_target_dependencies',
            'install(TARGETS'
        ]
        
        all_found = True
        for item in required_items:
            if item in content:
                print(f"  ✓ Found: {item}")
            else:
                print(f"  ✗ Missing: {item}")
                all_found = False
        
        return all_found
    except Exception as e:
        print(f"  ✗ Error reading CMakeLists.txt: {e}")
        return False

def check_uavlink_header(filepath):
    """Check UAVLink header for required definitions."""
    try:
        with open(filepath, 'r') as f:
            content = f.read()
        
        required_items = [
            'UAVLINK_MSG_ID_STATE',
            'UAVLINK_MSG_ID_POSITION_FEEDBACK',
            'UAVLINK_MSG_ID_DRONE_STATUS',
            'UAVLINK_MSG_ID_COMMAND',
            'UAVLINK_CMD_ARM_DISARM',
            'UAVLINK_CMD_TAKEOFF',
            'UAVLINK_CMD_LAND',
            'struct UAVLinkCommand',
            'struct UAVLinkDroneStatus',
            'serialize_drone_status',
            'deserialize_command'
        ]
        
        all_found = True
        for item in required_items:
            if item in content:
                print(f"  ✓ Found: {item}")
            else:
                print(f"  ✗ Missing: {item}")
                all_found = False
        
        return all_found
    except Exception as e:
        print(f"  ✗ Error reading uavlink.h: {e}")
        return False

def main():
    """Main test function."""
    print("=" * 60)
    print("WICOM ROS2 UDP Bridge Package Validation")
    print("=" * 60)
    print()
    
    # Determine package root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    pkg_root = os.path.join(script_dir, 'wicom_udp')
    
    if not os.path.exists(pkg_root):
        print(f"✗ Package directory not found: {pkg_root}")
        return 1
    
    print(f"Package root: {pkg_root}")
    print()
    
    all_checks_passed = True
    
    # Check required files
    print("Checking required files...")
    print("-" * 60)
    
    required_files = {
        'package.xml': os.path.join(pkg_root, 'package.xml'),
        'CMakeLists.txt': os.path.join(pkg_root, 'CMakeLists.txt'),
        'uavlink.h': os.path.join(pkg_root, 'include/wicom_udp/uavlink.h'),
        'udp_server_node.h': os.path.join(pkg_root, 'include/wicom_udp/udp_server_node.h'),
        'udp_server_node.cpp': os.path.join(pkg_root, 'src/udp_server_node.cpp'),
        'udp_server_main.cpp': os.path.join(pkg_root, 'src/udp_server_main.cpp'),
        'udp_server.launch.py': os.path.join(pkg_root, 'launch/udp_server.launch.py'),
        'udp_server.yaml': os.path.join(pkg_root, 'config/udp_server.yaml'),
    }
    
    for name, filepath in required_files.items():
        if not check_file_exists(filepath, name):
            all_checks_passed = False
    
    print()
    
    # Validate package.xml
    print("Validating package.xml...")
    print("-" * 60)
    if not check_package_xml(required_files['package.xml']):
        all_checks_passed = False
    print()
    
    # Validate CMakeLists.txt
    print("Validating CMakeLists.txt...")
    print("-" * 60)
    if not check_cmake_lists(required_files['CMakeLists.txt']):
        all_checks_passed = False
    print()
    
    # Validate UAVLink header
    print("Validating UAVLink protocol definitions...")
    print("-" * 60)
    if not check_uavlink_header(required_files['uavlink.h']):
        all_checks_passed = False
    print()
    
    # Summary
    print("=" * 60)
    if all_checks_passed:
        print("✓ All checks passed!")
        print()
        print("Next steps:")
        print("1. Install ROS2 Humble: source /opt/ros/humble/setup.bash")
        print("2. Install px4_msgs package")
        print("3. Build the package: colcon build --packages-select wicom_udp")
        print("4. Source workspace: source install/setup.bash")
        print("5. Run the node: ros2 launch wicom_udp udp_server.launch.py")
        return 0
    else:
        print("✗ Some checks failed!")
        print("Please review the errors above.")
        return 1

if __name__ == '__main__':
    sys.exit(main())
