find_package(Qt6 REQUIRED COMPONENTS Test Core5Compat)

# Unit tests link the same sources as the app (no QML registration needed).
qt_add_executable(playertest
    playertest.cpp
    ${CMAKE_SOURCE_DIR}/src/player/PlaylistController.cpp
    ${CMAKE_SOURCE_DIR}/src/player/SubtitleTrack.cpp
)
target_include_directories(playertest PRIVATE ${CMAKE_SOURCE_DIR}/src/player)
target_link_libraries(playertest PRIVATE Qt6::Core Qt6::Test Qt6::Core5Compat)

qt_add_executable(dlnatest
    dlnatest.cpp
    ${CMAKE_SOURCE_DIR}/src/dlna/SoapClient.cpp
    ${CMAKE_SOURCE_DIR}/src/dlna/AvTransportClient.cpp
    ${CMAKE_SOURCE_DIR}/src/dlna/MediaHttpServer.cpp
)
target_include_directories(dlnatest PRIVATE ${CMAKE_SOURCE_DIR}/src/dlna)
target_link_libraries(dlnatest PRIVATE Qt6::Core Qt6::Network Qt6::Test)

add_test(NAME playertest COMMAND playertest)
add_test(NAME dlnatest COMMAND dlnatest)