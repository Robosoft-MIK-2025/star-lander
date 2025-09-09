# ~/.bashrc: executed by bash(1) for non-login shells.
# see /usr/share/doc/bash/examples/startup-files (in the package bash-doc)
# for examples

# If not running interactively, don't do anything
[ -z "$PS1" ] && return

# don't put duplicate lines in the history. See bash(1) for more options
# ... or force ignoredups and ignorespace
HISTCONTROL=ignoredups:ignorespace

# append to the history file, don't overwrite it
shopt -s histappend

# for setting history length see HISTSIZE and HISTFILESIZE in bash(1)
HISTSIZE=1000
HISTFILESIZE=2000

# check the window size after each command and, if necessary,
# update the values of LINES and COLUMNS.
shopt -s checkwinsize

# make less more friendly for non-text input files, see lesspipe(1)
[ -x /usr/bin/lesspipe ] && eval "$(SHELL=/bin/sh lesspipe)"

# set variable identifying the chroot you work in (used in the prompt below)
if [ -z "$debian_chroot" ] && [ -r /etc/debian_chroot ]; then
    debian_chroot=$(cat /etc/debian_chroot)
fi

# set a fancy prompt (non-color, unless we know we "want" color)
case "$TERM" in
    xterm-color) color_prompt=yes;;
esac

# uncomment for a colored prompt, if the terminal has the capability; turned
# off by default to not distract the user: the focus in a terminal window
# should be on the output of commands, not on the prompt
#force_color_prompt=yes

if [ -n "$force_color_prompt" ]; then
    if [ -x /usr/bin/tput ] && tput setaf 1 >&/dev/null; then
	# We have color support; assume it's compliant with Ecma-48
	# (ISO/IEC-6429). (Lack of such support is extremely rare, and such
	# a case would tend to support setf rather than setaf.)
	color_prompt=yes
    else
	color_prompt=
    fi
fi

if [ "$color_prompt" = yes ]; then
    PS1='${debian_chroot:+($debian_chroot)}\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '
else
    PS1='${debian_chroot:+($debian_chroot)}\u@\h:\w\$ '
fi
unset color_prompt force_color_prompt

# If this is an xterm set the title to user@host:dir
case "$TERM" in
xterm*|rxvt*)
    PS1="\[\e]0;${debian_chroot:+($debian_chroot)}\u@\h: \w\a\]$PS1"
    ;;
*)
    ;;
esac

# enable color support of ls and also add handy aliases
if [ -x /usr/bin/dircolors ]; then
    test -r ~/.dircolors && eval "$(dircolors -b ~/.dircolors)" || eval "$(dircolors -b)"
    alias ls='ls --color=auto'
    #alias dir='dir --color=auto'
    #alias vdir='vdir --color=auto'

    alias grep='grep --color=auto'
    alias fgrep='fgrep --color=auto'
    alias egrep='egrep --color=auto'
fi

# some more ls aliases
alias ll='ls -alF'
alias la='ls -A'
alias l='ls -CF'

# Alias definitions.
# You may want to put all your additions into a separate file like
# ~/.bash_aliases, instead of adding them here directly.
# See /usr/share/doc/bash-doc/examples in the bash-doc package.

if [ -f ~/.bash_aliases ]; then
    . ~/.bash_aliases
fi

# enable programmable completion features (you don't need to enable
# this, if it's already enabled in /etc/bash.bashrc and /etc/profile
# sources /etc/bash.bashrc).
#if [ -f /etc/bash_completion ] && ! shopt -oq posix; then
#    . /etc/bash_completion
#fi

source /opt/ros/humble/setup.bash
source install/setup.bash || true

export DISPLAY=:1
export XAUTHORITY=/home/mobile/.Xauthority

export XDG_RUNTIME_DIR=/tmp/runtime-$(id -u)
mkdir -p $XDG_RUNTIME_DIR
chmod 700 $XDG_RUNTIME_DIR

export USER=root

# TODO: enable where it possible this variable due to best practice for change
export CURRENT_ROS_WS=/root/ros2_px4_ws

export GAZEBO_MODEL_PATH=$GAZEBO_MODEL_PATH:"$CURRENT_ROS_WS/src/gz_pkg/models"

export PX4_GZ_MODELS="$CURRENT_ROS_WS/src/gz_pkg/models"
export PX4_GZ_WORLDS="$CURRENT_ROS_WS/src/gz_pkg/worlds"

# force gazebo to use localhost communication
# to use "gz sim -v 4 /root/ros2_px4_ws/src/gz_pkg/worlds/empty_world.sdf"
export GZ_IP=127.0.0.1
export GZ_TRANSPORT_LOCALHOST_ONLY=1

export GZ_SIM_RESOURCE_PATH="$CURRENT_ROS_WS/src/gz_pkg/models":$GZ_SIM_RESOURCE_PATH
export GZ_SIM_RESOURCE_PATH="$CURRENT_ROS_WS/src/gz_pkg/worlds":$GZ_SIM_RESOURCE_PATH
export GZ_SIM_RESOURCE_PATH="$CURRENT_ROS_WS/src/gz_pkg":${GZ_SIM_RESOURCE_PATH}

export GZ_SIM_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu:${GZ_SIM_PLUGIN_PATH}
export GZ_SIM_SYSTEM_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/gz-sim-8/plugins:${GZ_SIM_SYSTEM_PLUGIN_PATH}

export GZ_PLUGIN_PATH=/opt/ros/humble/lib:/usr/lib/x86_64-linux-gnu/gz-sim-8/plugins:$GZ_PLUGIN_PATH

export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu/gz-sim-8/plugins:$LD_LIBRARY_PATH