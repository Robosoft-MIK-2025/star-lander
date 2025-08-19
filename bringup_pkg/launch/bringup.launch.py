from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    return LaunchDescription([
        # Запуск камеры
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                PathJoinSubstitution([
                    FindPackageShare('camera_pkg'),
                    'launch',
                    'camera.launch.py'
                ])
            )
        ),
        # Запуск AprilTag
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                PathJoinSubstitution([
                    FindPackageShare('apriltag_pkg'),
                    'launch',
                    'apriltag.launch.py'
                ])
            )
        ),
        # Запуск RViz2
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                PathJoinSubstitution([
                    FindPackageShare('rviz_pkg'),
                    'launch',
                    'rviz.launch.py'
                ])
            )
        )
    ])
