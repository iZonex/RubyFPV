#!/bin/bash

# Check if the script is running on Ubuntu
if [[ "$(lsb_release -is)" == "Ubuntu" ]]; then
  echo "Running on Ubuntu. Installing dependencies..."
  sudo apt-get update
  sudo apt-get install -y libcairo2-dev build-essential libpcap-dev libdrv-dev pkg-config
else
  echo "Not running on Ubuntu. Skipping dependency installation."
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
mkdir -p "$FOLDER_BINARIES" "$FOLDER_CONFIG" "$FOLDER_CONFIG_MODELS" "$FOLDER_LOGS" "$FOLDER_MEDIA" "$FOLDER_OSD_PLUGINS" "$FOLDER_CORE_PLUGINS" "$FOLDER_UPDATES" "$FOLDER_RUBY_TEMP" "$FOLDER_USB_MOUNT" "$FOLDER_TEMP_VIDEO_MEM" "$FOLDER_WINDOWS_PARTITION"

# Compile the project
make all RUBY_BUILD_ENV=steamdeck

echo "Directories created and project compiled relative to the current directory."