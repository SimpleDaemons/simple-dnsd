# CMakeLists.txt for simple-dnsd
# Simple DNS Daemon - An authoritative DNS server
# Copyright 2026 SimpleDaemons

cmake_minimum_required(VERSION 3.16)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
endif()
project(simple-dnsd VERSION 0.4.0 LANGUAGES CXX)

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
option(ENABLE_SSL "Enable SSL/TLS support (used from 0.10.0 TSIG)" ON)
option(ENABLE_JSON "Enable JSON support (used from 0.9.0 REST)" ON)
option(ENABLE_SQLITE "Enable SQLite gsql backend (used from 0.6.0)" ON)
option(ENABLE_POSTGRES "Enable PostgreSQL gsql backend (used from 0.7.0)" ON)
option(ENABLE_MYSQL "Enable MySQL/MariaDB gsql backend (used from 0.7.0)" ON)
option(ENABLE_STATIC_LINKING "Enable static linking for self-contained binaries" OFF)

# MacPorts on macOS (/opt/local); FreeBSD ports (/usr/local).
if(APPLE)
    foreach(_dns_prefix IN ITEMS
            "/opt/local"
            "/opt/local/libexec/openssl3"
            "/opt/local/lib/postgresql15"
            "/opt/local/lib/mysql")
        if(EXISTS "${_dns_prefix}")
            list(PREPEND CMAKE_PREFIX_PATH "${_dns_prefix}")
        endif()
    endforeach()
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    list(PREPEND CMAKE_PREFIX_PATH "/usr/local")
endif()

find_package(Threads REQUIRED)

if(ENABLE_SSL)
    find_package(OpenSSL QUIET COMPONENTS SSL Crypto)
    if(NOT OpenSSL_FOUND)
        message(WARNING "OpenSSL not found. SSL option disabled until 0.10.0 needs it.")
        set(ENABLE_SSL OFF)
    endif()
endif()

if(ENABLE_JSON)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(JSONCPP jsoncpp)
    endif()
    if(NOT JSONCPP_FOUND)
        message(WARNING "jsoncpp not found. REST API JSON support will be disabled.")
        set(ENABLE_JSON OFF)
    endif()
endif()

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

if(ENABLE_SSL)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(${PROJECT_NAME}_lib PUBLIC SIMPLE_DNSD_SSL=1)
endif()

if(ENABLE_JSON)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${JSONCPP_LIBRARIES})
    target_include_directories(${PROJECT_NAME}_lib PUBLIC ${JSONCPP_INCLUDE_DIRS})
    target_compile_options(${PROJECT_NAME}_lib PRIVATE ${JSONCPP_CFLAGS_OTHER})
    target_link_directories(${PROJECT_NAME}_lib PUBLIC ${JSONCPP_LIBRARY_DIRS})
    target_compile_definitions(${PROJECT_NAME}_lib PUBLIC SIMPLE_DNSD_JSON=1)
endif()

if(ENABLE_SQLITE)
    find_package(SQLite3 QUIET)
    set(_dns_sqlite_ok OFF)
    if(TARGET SQLite3::SQLite3)
        target_link_libraries(${PROJECT_NAME}_lib PUBLIC SQLite3::SQLite3)
        set(_dns_sqlite_ok ON)
    elseif(SQLite3_FOUND AND SQLite3_LIBRARIES)
        target_include_directories(${PROJECT_NAME}_lib PUBLIC ${SQLite3_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${SQLite3_LIBRARIES})
        set(_dns_sqlite_ok ON)
    endif()
    if(NOT _dns_sqlite_ok)
        find_package(unofficial-sqlite3 CONFIG QUIET)
        if(TARGET unofficial::sqlite3::sqlite3)
            target_link_libraries(${PROJECT_NAME}_lib PUBLIC unofficial::sqlite3::sqlite3)
            set(_dns_sqlite_ok ON)
        endif()
    endif()
    if(_dns_sqlite_ok)
        message(STATUS "SQLite3 found (backend sources land in 0.6.0)")
    else()
        message(WARNING "SQLite3 not found. sqlite backend will be unavailable.")
        set(ENABLE_SQLITE OFF)
    endif()
endif()

if(ENABLE_POSTGRES)
    find_package(PostgreSQL QUIET)
    if(PostgreSQL_FOUND)
        target_include_directories(${PROJECT_NAME}_lib PUBLIC ${PostgreSQL_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${PostgreSQL_LIBRARIES})
        target_compile_definitions(${PROJECT_NAME}_lib PUBLIC SIMPLE_DNSD_POSTGRES=1)
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(LIBPQ libpq)
        endif()
        if(LIBPQ_FOUND)
            target_include_directories(${PROJECT_NAME}_lib PUBLIC ${LIBPQ_INCLUDE_DIRS})
            target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${LIBPQ_LIBRARIES})
            target_link_directories(${PROJECT_NAME}_lib PUBLIC ${LIBPQ_LIBRARY_DIRS})
            target_compile_definitions(${PROJECT_NAME}_lib PUBLIC SIMPLE_DNSD_POSTGRES=1)
        else()
            find_package(unofficial-libpq CONFIG QUIET)
            if(TARGET unofficial::libpq::libpq)
                target_link_libraries(${PROJECT_NAME}_lib PUBLIC unofficial::libpq::libpq)
                target_compile_definitions(${PROJECT_NAME}_lib PUBLIC SIMPLE_DNSD_POSTGRES=1)
            else()
                message(WARNING "libpq not found. postgres backend will be unavailable.")
                set(ENABLE_POSTGRES OFF)
            endif()
        endif()
    endif()
endif()

if(ENABLE_MYSQL)
    find_path(MARIADB_INCLUDE_DIR mysql.h PATH_SUFFIXES mysql mariadb)
    find_library(MARIADB_LIBRARY NAMES mariadb mariadbclient mysqlclient libmariadb
        PATH_SUFFIXES mysql mariadb)
    if(NOT (MARIADB_INCLUDE_DIR AND MARIADB_LIBRARY))
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(LIBMARIADB libmariadb)
            if(NOT LIBMARIADB_FOUND)
                pkg_check_modules(LIBMARIADB mysqlclient)
            endif()
        endif()
        if(LIBMARIADB_FOUND)
            set(MARIADB_INCLUDE_DIR ${LIBMARIADB_INCLUDE_DIRS})
            set(MARIADB_LIBRARY ${LIBMARIADB_LIBRARIES})
            target_link_directories(${PROJECT_NAME}_lib PUBLIC ${LIBMARIADB_LIBRARY_DIRS})
        endif()
    endif()
    if(MARIADB_INCLUDE_DIR AND MARIADB_LIBRARY)
        target_include_directories(${PROJECT_NAME}_lib PUBLIC ${MARIADB_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${MARIADB_LIBRARY})
        target_compile_definitions(${PROJECT_NAME}_lib PUBLIC SIMPLE_DNSD_MYSQL=1)
    else()
        find_package(unofficial-libmariadb CONFIG QUIET)
        if(TARGET unofficial::libmariadb)
            target_link_libraries(${PROJECT_NAME}_lib PUBLIC unofficial::libmariadb)
            target_compile_definitions(${PROJECT_NAME}_lib PUBLIC SIMPLE_DNSD_MYSQL=1)
        else()
            message(WARNING "MariaDB/MySQL client not found. mysql backend will be unavailable.")
            set(ENABLE_MYSQL OFF)
        endif()
    endif()
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
    function(add_dns_test name source)
        add_executable(${name} ${source})
        target_link_libraries(${name} PRIVATE ${PROJECT_NAME}_lib)
        target_compile_options(${name} PRIVATE ${_dns_warning_flags})
        if(MSVC)
            target_compile_options(${name} PRIVATE /UNDEBUG)
        else()
            target_compile_options(${name} PRIVATE -UNDEBUG)
        endif()
        add_test(NAME ${name} COMMAND ${name})
    endfunction()
    add_dns_test(test_dns_config tests/unit/test_dns_config.cpp)
    add_dns_test(test_dns_name tests/unit/test_dns_name.cpp)
    add_dns_test(test_dns_codec tests/unit/test_dns_codec.cpp)
    add_dns_test(test_dns_backend tests/unit/test_dns_backend.cpp)
    add_dns_test(test_dns_engine tests/unit/test_dns_engine.cpp)
    add_dns_test(test_dns_server tests/integration/test_dns_server.cpp)
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
message(STATUS "  SSL support: ${ENABLE_SSL}")
message(STATUS "  JSON support: ${ENABLE_JSON}")
message(STATUS "  SQLite support: ${ENABLE_SQLITE}")
message(STATUS "  PostgreSQL support: ${ENABLE_POSTGRES}")
message(STATUS "  MySQL/MariaDB support: ${ENABLE_MYSQL}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
