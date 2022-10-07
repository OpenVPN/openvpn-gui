option(CLI_OVPN3 "Build ${PROJECT_NAME} with OpenVPN3 support" OFF)

cmake_minimum_required(VERSION 3.10)

if(NOT VCPKG_MANIFEST_DIR)
    set(VCPKG_MANIFEST_DIR ${CMAKE_SOURCE_DIR}/vcpkg_manifests/openssl_3)
endif()

project(openvpn-gui C CXX)

add_executable(${PROJECT_NAME} WIN32
    access.c
    echo.c
    env_set.c
    localization.c
    main.c
    manage.c
    misc.c
    openvpn.c
    openvpn_config.c
    options.c
    passphrase.c
    proxy.c
    registry.c
    save_pass.c
    scripts.c
    service.c
    tray.c
    viewlog.c
    as.c
    pkcs11.c
    config_parser.c
    res/openvpn-gui-res.rc)

find_package(OpenSSL REQUIRED)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /guard:cf")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /guard:cf /DYNAMICBASE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Qspectre")

if(NOT (${CMAKE_C_COMPILER_ARCHITECTURE_ID} STREQUAL "ARM64"))
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /CETCOMPAT")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenSSL::SSL
    Wtsapi32.lib
    Netapi32.lib
    ws2_32.lib
    Winhttp.lib
    Secur32.lib
    Comctl32.lib
    Crypt32.lib
    Shlwapi.lib
    Advapi32.lib
    Shell32.lib
    Gdi32.lib
    Comdlg32.lib
    Ole32.lib
    Cryptui.lib
    Wininet.lib)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
set(CMAKE_RC_FLAGS "/D \"HAVE_CONFIG_H\" /D \"_MSC_VER\"")

function(get_version_from_configure regex_arg ret)
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/configure.ac" VERSION_STR REGEX ${regex_arg})
    string(REGEX MATCH ${regex_arg} VERSION_STR "${VERSION_STR}")
    set("${ret}" ${CMAKE_MATCH_1} PARENT_SCOPE)
endfunction()

get_version_from_configure("_GUI_VERSION_MAJOR\\], \\[([0-9]+)\\]" GUI_VERSION_MAJOR)
get_version_from_configure("_GUI_VERSION_MINOR\\], \\[([0-9]+)\\]" GUI_VERSION_MINOR)

configure_file(config-msvc.h.in config.h)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _UNICODE
    UNICODE
    WIN32_LEAN_AND_MEAN
    HAVE_CONFIG_H)

if (${CLI_OVPN3})
    find_package(json-c REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        json-c::json-c)

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ENABLE_OVPN3)
endif ()

if (NOT PROJECT_NAME_PLAP)
    set(PROJECT_NAME_PLAP "libopenvpn_plap")
endif()

add_library(${PROJECT_NAME_PLAP} SHARED
    localization.c
    manage.c
    misc.c
    openvpn.c
    openvpn_config.c
    options.c
    proxy.c
    pkcs11.c
    registry.c
    config_parser.c
    plap/ui_glue.c
    plap/stub.c
    plap/plap_common.c
    plap/plap_provider.c
    plap/plap_connection.c
    plap/plap_dll.c
    plap/openvpn_plap.def
    plap/openvpn-plap-res.rc)

target_link_libraries(${PROJECT_NAME_PLAP} PRIVATE
    ws2_32.lib
    Comctl32.lib
    Winhttp.lib
    Crypt32.lib
    Ole32.lib
    Shlwapi.lib
    Secur32.lib
    Gdi32.lib
    Cryptui.lib
    Rpcrt4.lib)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
target_include_directories(${PROJECT_NAME_PLAP} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME_PLAP} PRIVATE
    _UNICODE
    UNICODE
    WIN32_LEAN_AND_MEAN
    HAVE_CONFIG_H)

if (NOT TEST_PLAP_EXE)
    set(TEST_PLAP_EXE "test_plap")
endif()

add_executable(${TEST_PLAP_EXE}
    plap/test_plap.cpp
    plap/plap_common.c)

target_link_libraries(${TEST_PLAP_EXE} PRIVATE
    Rpcrt4
    Ole32
    Gdi32)

target_include_directories(${TEST_PLAP_EXE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})
target_compile_definitions(${TEST_PLAP_EXE} PRIVATE
    _UNICODE
    UNICODE
    WIN32_LEAN_AND_MEAN
    HAVE_CONFIG_H)

if(MSVC)
    # work around msvc generator Debug/Release directory ugliness, doesn't apply for Ninja
    set_target_properties(${PROJECT_NAME} ${PROJECT_NAME_PLAP} ${TEST_PLAP_EXE} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<0:>
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<0:>
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<0:>)
endif(MSVC)

set_target_properties(${TEST_PLAP_EXE} PROPERTIES
                      LINK_FLAGS " /MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_SOURCE_DIR}/plap/test-plap.manifest ")
