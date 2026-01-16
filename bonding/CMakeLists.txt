# CMakeLists.txt for bonding modules
cmake_minimum_required(VERSION 3.10)

# Define bonding library target
add_library(bonding STATIC
    src/config_parser.c
    src/bonding_manager.c
    src/openvpn_manager.c
    src/packet_distributor.c
    src/nic_detector.c
    service/bonding_service.c
    service/service_ipc.c
    ui/bonding_dialog.c
    ui/bonding_status.c
    utils/logging.c
    utils/routing_helper.c
)

# Set C99 standard
set_property(TARGET bonding PROPERTY C_STANDARD 99)

# Include directories
target_include_directories(bonding PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/service
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

# Link Windows networking libraries
target_link_libraries(bonding PRIVATE
    ws2_32.lib
    iphlpapi.lib
    advapi32.lib
)

# Compiler flags
target_compile_definitions(bonding PRIVATE
    _UNICODE
    UNICODE
    WIN32_LEAN_AND_MEAN
)

# Tests
add_executable(bonding_tests
    tests/test_config_parser.c
)

target_link_libraries(bonding_tests PRIVATE bonding)

target_include_directories(bonding_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
