from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    # Get the package share directory
    pkg_share = get_package_share_directory('wicom_udp')
    
    # Declare launch arguments
    port_arg = DeclareLaunchArgument(
        'port',
        default_value='12345',
        description='UDP port for the server'
    )
    
    config_file_arg = DeclareLaunchArgument(
        'config_file',
        default_value=os.path.join(pkg_share, 'config', 'udp_server.yaml'),
        description='Path to the configuration file'
    )
    
    # Create the UDP server node
    udp_server_node = Node(
        package='wicom_udp',
        executable='udp_server_node',
        name='udp_server_node',
        output='screen',
        parameters=[
            LaunchConfiguration('config_file'),
            {'port': LaunchConfiguration('port')}
        ]
    )
    
    return LaunchDescription([
        port_arg,
        config_file_arg,
        udp_server_node
    ])
