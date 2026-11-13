cmake_minimum_required(VERSION 3.25)

project(dbhx-player
    VERSION 0.1.0
    DESCRIPTION "Cross-platform video player with DLNA casting"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# M0 only needs the UI stack. Multimedia / Network are added in M1.
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

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

qt_add_qml_module(dbhx-player
    URI Dbhx
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
        qml/chrome/MacTitleBar.qml
        qml/chrome/WindowsTitleBar.qml
    SOURCES
        src/app/AppContext.h
        src/app/AppContext.cpp
)

target_link_libraries(dbhx-player PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

# The generated QML type registration includes <AppContext.h> via __has_include,
# so the header's directory must be on the include path.
target_include_directories(dbhx-player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
)

set_target_properties(dbhx-player PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

install(TARGETS dbhx-player
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)