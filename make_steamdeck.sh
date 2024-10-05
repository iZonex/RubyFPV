#!/bin/bash

# Detect the Linux distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
else
    echo "Cannot detect the Linux distribution."
    exit 1
fi

# Install dependencies based on distribution
if [[ "$DISTRO" == "ubuntu" ]]; then
    echo "Running on Ubuntu. Installing dependencies..."
    sudo apt-get update
    sudo apt-get install -y libcairo2-dev build-essential libpcap-dev pkg-config \
        libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-gfx-dev cmake
    # Note: 'libdrv-dev' does not seem to be a standard package in Ubuntu repositories.
    # Please ensure that this package is correct or provide the correct package name.
elif [[ "$DISTRO" == "arch" ]]; then
    echo "Running on Arch Linux. Installing dependencies..."
    sudo pacman -Syu --noconfirm
    sudo pacman -S --noconfirm base-devel cairo libpcap pkgconf \
        sdl2 sdl2_image sdl2_ttf sdl2_gfx cmake steam
    # Note: 'libdrv-dev' does not have a direct equivalent in Arch Linux repositories.
    # Please ensure that this package is correct or provide the correct package name.
else
    echo "Unsupported Linux distribution: $DISTRO"
    echo "Skipping dependency installation."
fi

# Define the base folder relative to the current directory
FOLDER_BINARIES="$(pwd)/"
FOLDER_CONFIG="${FOLDER_BINARIES}config/"
FOLDER_CONFIG_MODELS="${FOLDER_CONFIG}models/"
FOLDER_VEHICLE_HISTORY="${FOLDER_CONFIG_MODELS}history-%d/"
FOLDER_LOGS="${FOLDER_BINARIES}logs/"
FOLDER_MEDIA="${FOLDER_BINARIES}media/"
FOLDER_MEDIA_VEHICLE_DATA="${FOLDER_MEDIA}vehicle-%u/"
FOLDER_OSD_PLUGINS="${FOLDER_BINARIES}plugins/osd/"
FOLDER_CORE_PLUGINS="${FOLDER_BINARIES}plugins/core/"
FOLDER_UPDATES="${FOLDER_BINARIES}updates/"
FOLDER_RUBY_TEMP="${FOLDER_BINARIES}tmp/"
FOLDER_USB_MOUNT="${FOLDER_RUBY_TEMP}tmpusbfiles/"
FOLDER_TEMP_VIDEO_MEM="${FOLDER_RUBY_TEMP}memdisk/"
FOLDER_WINDOWS_PARTITION="$(pwd)/config/"

# Create the necessary directories
mkdir -p "$FOLDER_BINARIES" "$FOLDER_CONFIG" "$FOLDER_CONFIG_MODELS" "$FOLDER_LOGS" "$FOLDER_MEDIA" \
    "$FOLDER_OSD_PLUGINS" "$FOLDER_CORE_PLUGINS" "$FOLDER_UPDATES" "$FOLDER_RUBY_TEMP" \
    "$FOLDER_USB_MOUNT" "$FOLDER_TEMP_VIDEO_MEM" "$FOLDER_WINDOWS_PARTITION"

# Compile the project
make clean
make all RUBY_BUILD_ENV=steamdeck

echo "Dependencies installed, directories created, and project compiled relative to the current directory."