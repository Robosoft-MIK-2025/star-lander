from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='apriltag_ros',
            executable='apriltag_node',
            name='apriltag_node',
            output='screen',
            remappings=[
                ('image_rect', '/image_raw'),  # Топик изображения из v4l2_camera
                ('camera_info', '/camera_info')  # Калибровка камеры
            ],
            parameters=[
                PathJoinSubstitution([
                    FindPackageShare('apriltag_pkg'),
                    'config',
                    'tags.yaml'
                ])
            ],
            arguments=['--ros-args', '--log-level', 'apriltag_node:=debug']
        )
    ])