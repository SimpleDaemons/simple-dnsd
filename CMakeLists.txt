# CMakeLists.txt for simple-dnsd
# Simple DNS Daemon - An authoritative DNS server
# Copyright 2026 SimpleDaemons

cmake_minimum_required(VERSION 3.16)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
endif()
project(simple-dnsd VERSION 0.1.0 LANGUAGES CXX)

set(PROJECT_DESCRIPTION "Simple DNS Daemon - An authoritative DNS server")

if(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Installation prefix")
    endif()
endif()
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(PLATFORM_NAME "Windows")
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(PLATFORM_NAME "FreeBSD")
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_TESTS "Enable tests" ON)
option(ENABLE_PACKAGING "Enable package generation" ON)
option(ENABLE_STATIC_LINKING "Enable static linking for self-contained binaries" OFF)

find_package(Threads REQUIRED)

file(GLOB_RECURSE LIB_SOURCES
    "src/simple-dnsd/*.cpp"
    "src/simple-dnsd/*.c"
)
file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h")

add_library(${PROJECT_NAME}_lib STATIC ${LIB_SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME}_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC Threads::Threads)

if(WIN32)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC ws2_32)
    target_compile_definitions(${PROJECT_NAME}_lib PUBLIC
        _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

set(_dns_warning_flags)
if(MSVC)
    set(_dns_warning_flags /W3 /WX)
else()
    set(_dns_warning_flags -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND _dns_warning_flags -Wno-unused-private-field)
    endif()
endif()
target_compile_options(${PROJECT_NAME}_lib PRIVATE ${_dns_warning_flags})

if(ENABLE_STATIC_LINKING)
    set(BUILD_SHARED_LIBS OFF)
    if(MSVC)
        target_compile_options(${PROJECT_NAME}_lib PRIVATE /MT)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME}_lib PRIVATE -static-libgcc -static-libstdc++)
        target_link_options(${PROJECT_NAME}_lib PRIVATE -static-libgcc -static-libstdc++)
    endif()
endif()

set(DNS_BINARIES simple-dnsd simple-dnsutil)

foreach(_bin IN LISTS DNS_BINARIES)
    add_executable(${_bin} main/${_bin}.cpp)
    target_link_libraries(${_bin} PRIVATE ${PROJECT_NAME}_lib)
    target_compile_options(${_bin} PRIVATE ${_dns_warning_flags})
endforeach()

set(DNS_RUNTIME_TARGETS ${DNS_BINARIES})

if(APPLE)
    install(TARGETS ${DNS_RUNTIME_TARGETS}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    install(FILES deployment/launchd/com.simpledaemons.${PROJECT_NAME}.plist
        DESTINATION /Library/LaunchDaemons
    )
    install(DIRECTORY DESTINATION /var/lib/${PROJECT_NAME})
    install(DIRECTORY DESTINATION /var/log/${PROJECT_NAME})
    install(DIRECTORY DESTINATION /var/lib/${PROJECT_NAME}/zones)
else()
    install(TARGETS ${DNS_RUNTIME_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    install(DIRECTORY DESTINATION /var/lib/${PROJECT_NAME})
    install(DIRECTORY DESTINATION /var/log/${PROJECT_NAME})
    install(DIRECTORY DESTINATION /var/lib/${PROJECT_NAME}/zones)
    if(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        install(FILES deployment/systemd/${PROJECT_NAME}.service
            DESTINATION lib/systemd/system
        )
        install(FILES deployment/systemd/${PROJECT_NAME}.sysusers
            DESTINATION lib/sysusers.d
            RENAME ${PROJECT_NAME}.conf
        )
        install(FILES deployment/systemd/${PROJECT_NAME}.tmpfiles
            DESTINATION lib/tmpfiles.d
            RENAME ${PROJECT_NAME}.conf
        )
        install(FILES deployment/logrotate.d/${PROJECT_NAME}
            DESTINATION /etc/logrotate.d
        )
    endif()
endif()

if(NOT WIN32)
    set(_dns_confdir "/etc/${PROJECT_NAME}")
    install(DIRECTORY config/templates/
        DESTINATION ${_dns_confdir}/templates
        FILES_MATCHING PATTERN "*.conf"
    )
    install(DIRECTORY config/examples/
        DESTINATION ${_dns_confdir}/examples
    )
    install(FILES config/README.md
        DESTINATION ${_dns_confdir}
    )
    install(DIRECTORY schema/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/schema
        FILES_MATCHING PATTERN "*.sql"
    )
    install(DIRECTORY DESTINATION ${_dns_confdir}/tls)
    install(FILES LICENSE README.md CHANGELOG.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )
    install(DIRECTORY docs/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs
        FILES_MATCHING PATTERN "*.md"
    )
endif()

if(ENABLE_TESTS)
    enable_testing()
endif()

if(ENABLE_PACKAGING)
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VENDOR "SimpleDaemons")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_CONTACT "info@simpledaemons.com")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/simpledaemons/${PROJECT_NAME}")

    if(APPLE)
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/LICENSE.txt")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/readme.html")
        set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/welcome.html")
    else()
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    endif()

    if(WIN32)
        set(CPACK_GENERATOR "WIX;ZIP;NSIS")
        set(CPACK_WIX_PRODUCT_GUID "7c4a2e91-8b35-4d16-a9f0-c2d3e4f5a6b7")
        set(CPACK_WIX_UPGRADE_GUID "e1f2a3b4-c5d6-4789-8abc-def012345678")
        set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_WIX_PROPERTY_ARPNOREPAIR "1")
        set(CPACK_WIX_PROPERTY_ARPNOMODIFY "1")
        set(CPACK_ZIP_COMPONENT_INSTALL ON)
        set(CPACK_NSIS_DISPLAY_NAME "Simple DNS Daemon")
        set(CPACK_NSIS_PACKAGE_NAME "simple-dnsd")
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
            "CreateDirectory '$COMMON_APPDATA\\\\simple-dnsd'\\nCreateDirectory '$COMMON_APPDATA\\\\simple-dnsd\\\\logs'\\nCreateDirectory '$COMMON_APPDATA\\\\simple-dnsd\\\\zones'")
        set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
            "nsExec::ExecToLog 'sc stop simple-dnsd'\\nnsExec::ExecToLog 'sc delete simple-dnsd'")
    elseif(APPLE)
        set(CPACK_GENERATOR "DragNDrop;productbuild")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_PRODUCTBUILD_DOMAINS TRUE)
        set(CPACK_PRODUCTBUILD_DOMAINS_ANYWHERE TRUE)
        set(CPACK_PRODUCTBUILD_DOMAINS_ROOT TRUE)
        set(CPACK_PACKAGE_RELOCATABLE FALSE)
        set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
        set(CPACK_DMG_FORMAT "UDBZ")
        set(CPACK_PRODUCTBUILD_PACKAGE_NAME "${PROJECT_NAME}")
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
        set(CPACK_POSTFLIGHT_Unspecified_SCRIPT
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/pkg/scripts/postinstall")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(CPACK_GENERATOR "FREEBSD")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/")
        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_FREEBSD_PACKAGE_MAINTAINER "SimpleDaemons <info@simpledaemons.com>")
        set(CPACK_FREEBSD_PACKAGE_ORIGIN "net/${PROJECT_NAME}")
        set(CPACK_FREEBSD_PACKAGE_LICENSE "APACHE20")
        set(CPACK_FREEBSD_PACKAGE_RUNTIME_DEPENDS "")
    else()
        set(CPACK_GENERATOR "DEB;RPM")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/")
        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "SimpleDaemons <info@simpledaemons.com>")
        set(CPACK_DEBIAN_PACKAGE_SECTION "net")
        set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
        set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "systemd")
        set(CPACK_DEBIAN_PACKAGE_SUGGESTS "logrotate")
        set(CPACK_DEBIAN_PACKAGE_PROVIDES "${PROJECT_NAME}")
        set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/deb/postinst"
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/deb/prerm"
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/deb/postrm"
        )
        set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
        set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
        set(CPACK_RPM_PACKAGE_URL "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_RPM_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        set(CPACK_RPM_PACKAGE_REQUIRES "")
        set(CPACK_RPM_PACKAGE_SUGGESTS "systemd")
        set(CPACK_RPM_PACKAGE_PROVIDES "${PROJECT_NAME}")
        set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/preinstall.sh")
        set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/postinstall.sh")
        set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/preuninstall.sh")
        set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/postuninstall.sh")
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
        set(PACKAGE_ARCH "amd64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "aarch64")
        set(PACKAGE_ARCH "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "armv7hl")
        set(PACKAGE_ARCH "armhf")
    else()
        set(PACKAGE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    if(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(PLATFORM_ID "macos-apple")
        else()
            set(PLATFORM_ID "macos-intel")
        endif()
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(PLATFORM_ID "freebsd")
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
        set(CPACK_FREEBSD_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
    elseif(UNIX)
        set(PLATFORM_ID "linux-generic")
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
        set(CPACK_DEBIAN_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}.deb")
        set(CPACK_RPM_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}.rpm")
    else()
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_NAME}-${PACKAGE_ARCH}")
    endif()

    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
    set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-src")
    set(CPACK_SOURCE_IGNORE_FILES
        "/build/"
        "/dist/"
        "/.git/"
        "/.gitignore"
        "/.vscode/"
        "/.vs/"
        "/CMakeCache.txt"
        "/CMakeFiles/"
        "/cmake_install.cmake"
        "/Makefile"
        "/install_manifest.txt"
        "/compile_commands.json"
        "/CTestTestfile.cmake"
        "/_CPack_Packages/"
        "/.DS_Store"
        "/Thumbs.db"
    )
    include(CPack)
endif()

message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Platform: ${PLATFORM_NAME}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests enabled: ${ENABLE_TESTS}")
message(STATUS "  Packaging enabled: ${ENABLE_PACKAGING}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
