# Use ROS 2 Humble from Docker Hub as the base image
FROM osrf/ros:humble-desktop-full
# Set non-interactive frontend for debconf
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
    --fix-missing

# mine
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y \
    wget \
    v4l-utils \
    ros-${ROS_DISTRO}-rviz-default-plugins \
    ros-${ROS_DISTRO}-rqt-tf-tree

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
RUN mkdir -p /root/ros2_px4_ws/src

COPY Micro-XRCE-DDS-Agent /root/ros2_px4_ws/src/Micro-XRCE-DDS-Agent/
COPY PX4-Autopilot /root/ros2_px4_ws/src/PX4-Autopilot

RUN cd /root/ros2_px4_ws/src/Micro-XRCE-DDS-Agent && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    sudo make install && \
    sudo ldconfig /usr/local/lib/

# Это сделать не получилось
# cd /root/ros2_px4_ws/src \
#     && git clone https://github.com/PX4/PX4-Autopilot.git --recursive \

    # грёбанный костыль - против грёбанной защиты git
RUN git config --global safe.directory '*' \
    # && cd /root/ros2_px4_ws/src \
    # && ls -la \
    && cd /root/ros2_px4_ws/src/PX4-Autopilot \
    && bash ./Tools/setup/ubuntu.sh \
    && git submodule update --init --recursive

# Purge and remove conflicting system packages to avoid duplicates with ROS vendored versions
RUN apt-get purge -y libgtest-dev libgmock-dev liburdfdom-dev liburdfdom-headers-dev liburdfdom-tools || true \
    && rm -rf /usr/src/gtest /usr/src/gmock /usr/share/urdfdom


# Clean up
RUN apt-get clean && rm -rf /var/lib/apt/lists/*

# Initialize rosdep (run as user)
RUN rm -f /etc/ros/rosdep/sources.list.d/20-default.list && \
    sudo rosdep init || true \
    && rosdep update

RUN apt update -y \
    && apt upgrade -y \
    && apt install python3-venv -y

# mine
RUN echo "source /root/ros2_px4_ws/src/.bashrc" >> /root/.bashrc

# Добавляем source в .bashrc
RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> /home/$USERNAME/.bashrc

CMD ["bash"]  