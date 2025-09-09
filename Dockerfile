# Use ROS 2 Humble from Docker Hub as the base image
FROM osrf/ros:humble-desktop-full
# Set non-interactive frontend for debconf
ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=humble

ENV CURRENT_ROS_WS=/root/ros2_px4_ws

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

RUN usermod -s /bin/bash mobile

# Update and install necessary packages
RUN apt-get update && apt-get upgrade -y \
    && apt-get install -y nano sudo curl gnupg2 lsb-release net-tools python3-pip \
    && curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | apt-key add -

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
    ros-${ROS_DISTRO}-ros-gz \
    ros-${ROS_DISTRO}-ros-gz-bridge \
    ros-${ROS_DISTRO}-ros-gz-sim \
    ros-${ROS_DISTRO}-ros-gz-interfaces \
    ros-${ROS_DISTRO}-ros-ign-bridge \
    --fix-missing

# mine
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y \
    wget \
    # for AppImage extract
    libfuse2 \ 
    fuse \
    squashfs-tools \
    # for camera and ros2 additional packages
    v4l-utils \
    ros-${ROS_DISTRO}-rviz-default-plugins \
    ros-${ROS_DISTRO}-rqt-tf-tree \
    # instead of git repo for faster use [not supported, only ROS1]
    # ros-${ROS_DISTRO}-px4-msgs 

    ros-${ROS_DISTRO}-ros-gz \
    ros-${ROS_DISTRO}-ros-gz-bridge \
    ros-${ROS_DISTRO}-ros-gz-sim \
    ros-${ROS_DISTRO}-ros-gz-interfaces \

    ros-${ROS_DISTRO}-ros-ign-bridge

RUN sudo curl https://packages.osrfoundation.org/gazebo.gpg --output /usr/share/keyrings/pkgs-osrf-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/pkgs-osrf-archive-keyring.gpg] https://packages.osrfoundation.org/gazebo/ubuntu-stable $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/gazebo-stable.list > /dev/null \
    && sudo apt-get update -y \
    && sudo apt-get install -y gz-harmonic

# NEW: Добавляем зависимости для PX4 (из ubuntu.sh скрипта PX4)
RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    libopencv-dev \
    python3-empy \
    python3-jinja2 \
    python3-packaging \
    python3-psutil \
    python3-toml \
    python3-yaml \
    ninja-build \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools \
    gstreamer1.0-x \
    gstreamer1.0-alsa \
    gstreamer1.0-gl \
    gstreamer1.0-gtk3 \
    gstreamer1.0-qt5 \
    gstreamer1.0-pulseaudio \
    --fix-missing

# NEW: Устанавливаем pip-пакеты для PX4/ROS2 compat
RUN pip3 install --upgrade pip && \
    pip3 install empy==3.3.4 pyros-genmsg kconfiglib jsonschema
    #numpy<2.0 pyyaml requests pyulog cerberus coverage ifaddr markupsafe pyelftools wheel argcomplete


# NEW: Создаём workspace dir и копируем клонированные репо из build context
RUN mkdir -p ${CURRENT_ROS_WS}/src \
    && cd ${CURRENT_ROS_WS} \
    && git clone -b v2.4.2 https://github.com/eProsima/Micro-XRCE-DDS-Agent.git \
    && git clone https://github.com/PX4/PX4-Autopilot.git --recursive \
    # hand build px4_msgs
    && git clone https://github.com/PX4/px4_msgs.git  \
    && git clone https://github.com/PX4/px4_ros_com.git

# build and setup micro xrce agent 
RUN cd ${CURRENT_ROS_WS}/Micro-XRCE-DDS-Agent \
    && sed -i '98s|2.12|2.13|' CMakeLists.txt \
    && sed -i '99s|2.12.x|2.13.3|' CMakeLists.txt \
    && mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    sudo make install && \
    sudo ldconfig /usr/local/lib/

# QGroundControl install and setup
RUN wget https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl-x86_64.AppImage -O ${CURRENT_ROS_WS}/QGroundControl-x86_64.AppImage \
    && chmod +x ${CURRENT_ROS_WS}/QGroundControl-x86_64.AppImage \
    && usermod -aG dialout mobile \
    # && systemctl mask --now ModemManager.service \
    # На всякий случай, если директория не существует
    && mkdir -p /etc/systemd/system \
    && ln -sf /dev/null /etc/systemd/system/ModemManager.service


# setup and install PX4-Autopilot
    # грёбанный костыль - против грёбанной защиты git
RUN git config --global safe.directory '*' \
    # git config --global --unset safe.directory
    # git config --global --unset-all safe.directory
    && cd ${CURRENT_ROS_WS}/PX4-Autopilot \
    && bash ./Tools/setup/ubuntu.sh \
    && git submodule update --init --recursive

# Initialize rosdep (run as user)
RUN rm -f /etc/ros/rosdep/sources.list.d/20-default.list && \
    sudo rosdep init || true \
    && rosdep update

# install dependencies and build pkgs
RUN apt-get update \
    && cd ${CURRENT_ROS_WS} \
    && rosdep install --from-paths . --ignore-src -r -y \
    # from /bin/sh don't work
    # && . /opt/ros/${ROS_DISTRO}/setup.bash \
    # && colcon build --symlink-install --packages-select px4_msgs px4_ros_com
    && /bin/bash -c "source /opt/ros/${ROS_DISTRO}/setup.bash && colcon build --symlink-install --packages-select px4_msgs px4_ros_com  && . install/setup.bash"
    # unfortunately don't see tf_pkg apriltag_pkg bringup_pkg camera_pkg rviz_pkg to build it here, need to do it in runtime

# Clean up
RUN apt-get clean && rm -rf /var/lib/apt/lists/*

# mine
RUN echo "source ${CURRENT_ROS_WS}/src/.bashrc" >> /root/.bashrc

# Добавляем source в .bashrc
RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> /home/$USERNAME/.bashrc

CMD ["bash"]  