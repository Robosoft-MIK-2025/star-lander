# Use ROS 2 Humble from Docker Hub as the base image
FROM osrf/ros:humble-desktop-full
# Set non-interactive frontend fodebconf
ENV DEBIAN_FRONTEND=noninteractive

ENV ROS_DISTRO=humble

# Set arguments for user creation
ARG USERNAME=mobile
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# chsh -s /bin/bash
# sudo usermod -s /bin/bash mobile

# Update and install necessary packages
RUN apt-get update && apt-get upgrade -y \
    && apt-get install -y nano sudo curl gnupg2 lsb-release net-tools python3-pip \
    && curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | apt-key add - \
    # && echo "deb http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main" > /etc/apt/sources.list.d/ros2-latest.list

# Install slcan-utils from source if not available
RUN apt-get install -y git build-essential \
    && git clone https://github.com/linux-can/can-utils.git \
    && cd can-utils \
    && make \
    && make install

# Обновление и установка базовых пакетов
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y \
    sudo \
    git \
    python3-pip \
    ros-${ROS_DISTRO}-tf2-tools \
    ros-${ROS_DISTRO}-gazebo-ros \
    ros-${ROS_DISTRO}-robot-state-publisher \
    ros-${ROS_DISTRO}-joint-state-publisher \
    ros-${ROS_DISTRO}-xacro \
    ros-${ROS_DISTRO}-rviz2 \
    ros-${ROS_DISTRO}-hardware-interface \
    ros-${ROS_DISTRO}-transmission-interface \
    ros-${ROS_DISTRO}-urdf \
    ros-${ROS_DISTRO}-urdfdom \
    ros-${ROS_DISTRO}-urdfdom-headers \
    ros-${ROS_DISTRO}-urdf-tutorial \
    ros-${ROS_DISTRO}-apriltag-ros \
    ros-${ROS_DISTRO}-gz-ros2-control \
    ros-${ROS_DISTRO}-v4l2-camera \
    ros-${ROS_DISTRO}-camera-calibration \
    ros-${ROS_DISTRO}-gazebo-ros-pkgs \
    ros-${ROS_DISTRO}-nav2-bringup \
    libcanberra-gtk-module \
    libcanberra-gtk3-module \
    at-spi2-core \
    x11-apps \
    xauth \
    --fix-missing

# mine
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y \
    v4l-utils \
    ros-${ROS_DISTRO}-rviz-default-plugins \
    ros-${ROS_DISTRO}-rqt-tf-tree

# Clean up
RUN apt-get clean && rm -rf /var/lib/apt/lists/*

# Initialize rosdep (run as user)
RUN sudo rosdep init || true \
    && rosdep update

# mine
RUN echo "source /root/ros2_ws/src/.bashrc" >> /root/.bashrc

# Добавляем source в .bashrc
RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> /home/$USERNAME/.bashrc

CMD ["bash"]

