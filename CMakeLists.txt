cmake_minimum_required (VERSION 3.0.2)

project (openvpn-gui VERSION 11)

include (GNUInstallDirs)

set (PACKAGE "openvpn-gui")
set (PACKAGE_BUGREPORT "openvpn-devel@lists.sourceforge.net")
set (PACKAGE_NAME "OpenVPN GUI")
set (PACKAGE_STRING "${PACKAGE_NAME} ${PROJECT_VERSION_MAJOR}")
set (PACKAGE_TARNAME "openvpn-gui")
set (PACKAGE_URL "https://github.com/openvpn/openvpn-gui/")
set (PACKAGE_VERSION "${PROJECT_VERSION}")
set (PACKAGE_VERSION_RESOURCE "${PROJECT_VERSION_MAJOR},0,0,0")

if (NOT MAX_CONFIGS)
set (MAX_CONFIGS 50)
endif ()

find_package (OpenSSL 0.9.6)
if (OPENSSL_FOUND)
	option (DISABLE_CHANGE_PASSWORD "Disable password change support" OFF)
endif ()


configure_file (config.h.in.cmake ${CMAKE_SOURCE_DIR}/config.h)

set (openvpn_gui_RESOURCES
	res/openvpn-gui-res.rc)
	
set (openvpn_gui_SOURCES
	main.c main.h
	openvpn.c openvpn.h
	localization.c localization.h
	tray.c tray.h
	viewlog.c viewlog.h
	service.c service.h
	options.c options.h
	passphrase.c passphrase.h
	proxy.c proxy.h
	registry.c registry.h
	scripts.c scripts.h
	manage.c manage.h
	misc.c misc.h
	openvpn_config.c
	openvpn_config.h
	access.c access.h
	chartable.h
	save_pass.c save_pass.h
	openvpn-gui-res.h)
	
add_executable (openvpn-gui WIN32 ${openvpn_gui_SOURCES} ${openvpn_gui_RESOURCES})
target_include_directories (openvpn-gui PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_definitions (openvpn-gui PRIVATE _UNICODE;UNICODE;HAVE_CONFIG_H;WIN32_LEAN_AND_MEAN;_WIN32_WINNT=NTDDI_WINXP)
target_link_libraries (openvpn-gui PRIVATE ws2_32;comctl32;winhttp;wtsapi32;crypt32;netapi32;ole32;shlwapi;secur32)
if (NOT DISABLE_CHANGE_PASSWORD)
	target_include_directories (openvpn-gui PRIVATE ${OPENSSL_INCLUDE_DIR})
	target_link_libraries (openvpn-gui PUBLIC ${OPENSSL_LIBRARIES})
endif ()
if (MINGW)
	set_target_properties (openvpn-gui PROPERTIES LINK_FLAGS -municode)
endif ()

install (TARGETS openvpn-gui
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install (FILES COPYRIGHT.GPL COPYING DESTINATION ${CMAKE_INSTALL_DOCDIR})