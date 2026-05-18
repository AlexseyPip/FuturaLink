cmake_minimum_required(VERSION 3.10)
project(Messenger)

set(CMAKE_CXX_STANDARD 11)

# Определяем платформу
if(WIN32 OR MINGW OR CYGWIN)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    set(PLATFORM_LIBS ws2_32 comctl32 gdi32)
else()
    # Кросскомпиляция для Windows из Linux
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-win32)
    set(PLATFORM_LIBS ws2_32 comctl32 gdi32)
endif()

set(SOURCES
    src/main.cpp
    src/core/Logger.cpp
    src/core/HttpClient.cpp
    src/core/CryptoManager.cpp
    src/core/Database.cpp
    src/core/MessageServer.cpp
    src/core/NetworkClient.cpp
    src/gui/MainWindow.cpp
    src/gui/LoginDialog.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${PLATFORM_LIBS})

# Статическая линковка
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-static")