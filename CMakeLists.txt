# ESP32 NanoBot - Ultra-Lightweight AI Assistant for ESP32-S3
# Minimum CMake version required
cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project configuration
set(PROJECT_VER "1.0.0")
set(PROJECT_VER_NUMBER 10000)

# Partition table
set(IDF_PARTITION_TABLE "partitions.csv")

project(esp32_nanobot)
