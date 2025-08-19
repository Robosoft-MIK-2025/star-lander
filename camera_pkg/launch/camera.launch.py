from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration
from launch.actions import DeclareLaunchArgument

def generate_launch_description():
    return LaunchDescription([
        # Объявляем аргументы запуска (для гибкости, если параметры изменятся)
        DeclareLaunchArgument('video_device', default_value='/dev/video0', description='Path to video device'),
        DeclareLaunchArgument('image_size', default_value='[640,480]', description='Image resolution [width,height]'),
        DeclareLaunchArgument('output_encoding', default_value='yuv422_yuy2', description='Output image encoding'),  # yuv422_yuy2 bgr8 mjpeg rgb8
        DeclareLaunchArgument('camera_info_url', default_value='file:///root/ros2_ws/src/camera_pkg/config/ost.yaml', description='URL to camera calibration file'),
        # DeclareLaunchArgument('time_per_frame', default_value='[1,30]', description='Time per frame [numerator,denominator] for FPS'),

        # Запуск узла v4l2_camera_node с параметрами
        Node(
            package='v4l2_camera',
            executable='v4l2_camera_node',
            name='v4l2_camera_node',
            output='screen',
            parameters=[{
                'video_device': LaunchConfiguration('video_device'),
                'image_size': LaunchConfiguration('image_size'),
                'output_encoding': LaunchConfiguration('output_encoding'),
                'camera_info_url': LaunchConfiguration('camera_info_url'),
                # 'time_per_frame': [1, 30],
                'camera_frame_id': 'camera_link',
                # 'frame_rate': 30.0,
                # 'pixel_format': 'MJPG',
            }],
            # arguments=['--ros-args', '--log-level', 'v4l2_camera_node:=debug']
        ),
    ])