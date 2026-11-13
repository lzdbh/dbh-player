cmake_minimum_required(VERSION 3.25)

project(dbhx-player
    VERSION 0.1.0
    DESCRIPTION "Cross-platform video player with DLNA casting"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Multimedia Network)

qt_standard_project_setup()

qt_add_executable(dbhx-player
    src/main.cpp
)

# Keep an organized physical layout under qml/, but flatten the resource
# paths so every type lives at the module root (Dbhx). This makes
# loadFromModule("Dbhx", "Main") and same-module type references reliable.
set_source_files_properties(qml/Main.qml PROPERTIES
    QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(qml/theme/Theme.qml PROPERTIES
    QT_RESOURCE_ALIAS Theme.qml
    QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/chrome/MacTitleBar.qml PROPERTIES
    QT_RESOURCE_ALIAS MacTitleBar.qml)
set_source_files_properties(qml/chrome/WindowsTitleBar.qml PROPERTIES
    QT_RESOURCE_ALIAS WindowsTitleBar.qml)
set_source_files_properties(qml/player/PlayerView.qml PROPERTIES
    QT_RESOURCE_ALIAS PlayerView.qml)
set_source_files_properties(qml/player/ControlBar.qml PROPERTIES
    QT_RESOURCE_ALIAS ControlBar.qml)
set_source_files_properties(qml/cast/DeviceList.qml PROPERTIES
    QT_RESOURCE_ALIAS DeviceList.qml)
set_source_files_properties(qml/cast/CastPanel.qml PROPERTIES
    QT_RESOURCE_ALIAS CastPanel.qml)

qt_add_qml_module(dbhx-player
    URI Dbhx
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
        qml/chrome/MacTitleBar.qml
        qml/chrome/WindowsTitleBar.qml
        qml/player/PlayerView.qml
        qml/player/ControlBar.qml
        qml/cast/DeviceList.qml
        qml/cast/CastPanel.qml
    SOURCES
        src/app/AppContext.h
        src/app/AppContext.cpp
        src/player/PlayerController.h
        src/player/PlayerController.cpp
        src/dlna/SsdpDiscovery.h
        src/dlna/SsdpDiscovery.cpp
        src/dlna/DeviceParser.h
        src/dlna/DeviceParser.cpp
        src/dlna/DlnaDeviceModel.h
        src/dlna/DlnaDeviceModel.cpp
        src/dlna/SoapClient.h
        src/dlna/SoapClient.cpp
        src/dlna/AvTransportClient.h
        src/dlna/AvTransportClient.cpp
        src/dlna/RenderingControlClient.h
        src/dlna/RenderingControlClient.cpp
        src/dlna/MediaHttpServer.h
        src/dlna/MediaHttpServer.cpp
        src/dlna/CastManager.h
        src/dlna/CastManager.cpp
)

target_link_libraries(dbhx-player PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::Network
)

# The generated QML type registration includes each header via __has_include,
# so the header directories must be on the include path.
target_include_directories(dbhx-player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/player
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dlna
)

set_target_properties(dbhx-player PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER com.dbhx.player
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

install(TARGETS dbhx-player
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# macOS blocks local network (SSDP/multicast) sends from unsigned binaries
# with EHOSTUNREACH; ad-hoc sign every build so DLNA discovery works.
if(APPLE)
    add_custom_command(TARGET dbhx-player POST_BUILD
        COMMAND codesign --force -s - --identifier com.dbhx.player
                $<TARGET_BUNDLE_DIR:dbhx-player>
        COMMENT "Ad-hoc signing dbhx-player.app")
endif()