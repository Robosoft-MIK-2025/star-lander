from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():

    apriltag_config = os.path.join(
        get_package_share_directory('apriltag_pkg'),
        'config',
        'tags.yaml'
    )
    # wrong
    #     PathJoinSubstitution([
    #     FindPackageShare('apriltag_pkg'),
    #     'config',
    #     'tags.yaml'
    # ])

    return LaunchDescription([
        Node(
            package='apriltag_ros',
            executable='apriltag_node',
            name='apriltag',
            output='screen',
            remappings=[
                ('image_rect', '/image_raw'),  # Топик изображения из v4l2_camera
                ('camera_info', '/camera_info')  # Калибровка камеры
            ],
            parameters=[
                apriltag_config
            ],
            arguments=['--ros-args', '--log-level', 'apriltag:=ERROR'] # debug
        )
    ])