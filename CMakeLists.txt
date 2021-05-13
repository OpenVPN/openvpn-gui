cmake_minimum_required(VERSION 3.10)

set(VCPKG_FEATURE_FLAGS manifests)

project(openvpn-gui C)

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
    res/openvpn-gui-res.rc)

configure_file(config-msvc.h config.h COPYONLY)

find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenSSL::SSL
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
    Ole32.lib)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
set(CMAKE_RC_FLAGS "/D \"HAVE_CONFIG_H\" /D \"_MSC_VER\"")

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _UNICODE
    UNICODE
    WIN32_LEAN_AND_MEAN
    HAVE_CONFIG_H
    _CRT_NON_CONFORMING_WCSTOK)
