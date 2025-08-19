from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            output='screen',
            arguments=[ '-d', 
            PathJoinSubstitution([
                FindPackageShare('rviz_pkg'),
                'rviz',
                'apriltag.rviz'
            ]),
            #'--ros-args', '--log-level' #, 'rviz2:=debug'  # , 'debug' Добавь это для детальных логов (или 'DEBUG' в uppercase, если нужно)
            ]
        )
    ])