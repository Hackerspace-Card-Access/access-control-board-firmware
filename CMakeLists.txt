# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(access-control-board-firmware)

# Embed certificate files for OTA (optional)
if(CONFIG_EMBED_SERVER_CERT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/certs/server_cert.pem")
    target_add_binary_data(${PROJECT_NAME}.elf "certs/server_cert.pem" TEXT)
endif()

if(CONFIG_EMBED_CLIENT_CERT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/certs/client_cert.pem")
    target_add_binary_data(${PROJECT_NAME}.elf "certs/client_cert.pem" TEXT)
endif()

if(CONFIG_EMBED_CLIENT_CERT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/certs/client_key.pem")
    target_add_binary_data(${PROJECT_NAME}.elf "certs/client_key.pem" TEXT)
endif()