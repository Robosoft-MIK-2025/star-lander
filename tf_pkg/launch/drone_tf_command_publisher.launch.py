from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node



def generate_launch_description():
    return LaunchDescription([
        # TODO::
        # DeclareLaunchArgument(),
        Node(
            package='tf_pkg',
            executable='drone_tf_listener',
            name='listener',
            parameters=[
                {'use_sim_time': True}
            ]
        ),
    ])