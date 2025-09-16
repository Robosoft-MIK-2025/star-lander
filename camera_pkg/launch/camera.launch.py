from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration
from launch.actions import DeclareLaunchArgument
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    package_share_dir = get_package_share_directory('camera_pkg')
    camera_info_path = os.path.join(package_share_dir, 'config', 'ost.yaml')


    return LaunchDescription([
        # Объявляем аргументы запуска (для гибкости, если параметры изменятся)
        DeclareLaunchArgument('video_device', default_value='/dev/video0', description='Path to video device'),
        DeclareLaunchArgument('image_size', default_value='[640,480]', description='Image resolution [width,height]'),
        DeclareLaunchArgument('output_encoding', default_value='yuv422_yuy2', description='Output image encoding'),  # yuv422_yuy2 bgr8 mjpeg rgb8
        DeclareLaunchArgument('camera_info_url', default_value=f'file://{camera_info_path}', description='URL to camera calibration file'), # /root/ros2_px4_ws/src/camera_pkg/config/ost.yaml

        # Запуск узла v4l2_camera_node с параметрами
        # Node(
        #     package='v4l2_camera',
        #     executable='v4l2_camera_node',
        #     name='v4l2_camera_node',
        #     output='screen',
        #     parameters=[{
        #         'video_device': LaunchConfiguration('video_device'),
        #         'image_size': LaunchConfiguration('image_size'),
        #         'output_encoding': LaunchConfiguration('output_encoding'),
        #         'camera_info_url': LaunchConfiguration('camera_info_url'),
        #         'camera_frame_id': 'camera_link',

        #     }],
        #     # arguments=['--ros-args', '--log-level', 'v4l2_camera_node:=debug']
        # ),

#         ros2 run v4l2_camera v4l2_camera_node --ros-args \
#   -p video_device:=/dev/video0 \
#   -p image_size:=[640,480] \
#   -p output_encoding:=yuv422_yuy2 \
# #   -p camera_info_url:=file:///root/ros2_ws/src/camera_pkg/config/ost.yaml \
#   -p camera_frame_id:=camera_link

        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='static_tf_camera_link_next_to_map',
            arguments=['0.25', '0.25', '0', '0', '0', '0', 'map', 'x500_vision_0/vision_link/vision'] # vision_optical camera_link x500_vision_0/vision_link/vision
        ),
        # ros2 run tf2_ros static_transform_publisher 0.25 0.25 0 0 0 0 map camera_link

        Node(
            package='ros_gz_bridge',
            executable='parameter_bridge',
            name='image_camera_info_from_gz_to_ros',
            arguments=['/world/default/model/x500_vision_0/link/vision_link/sensor/vision/image@sensor_msgs/msg/Image[gz.msgs.Image', '/world/default/model/x500_vision_0/link/vision_link/sensor/vision/camera_info@sensor_msgs/msg/CameraInfo[gz.msgs.CameraInfo']
        )


# ros2 run ros_gz_bridge parameter_bridge /image@sensor_msgs/msg/Image[gz.msgs.Image /camera_info@sensor_msgs/msg/CameraInfo[gz.msgs.CameraInfo

# ros2 run ros_gz_bridge parameter_bridge /world/default/model/x500_vision_0/link/vision_link/sensor/vision/image@sensor_msgs/msg/Image[gz.msgs.Image /world/default/model/x500_vision_0/link/vision_link/sensor/vision/camera_info@sensor_msgs/msg/CameraInfo[gz.msgs.CameraInfo
    ])