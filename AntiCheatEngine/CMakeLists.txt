cmake_minimum_required(VERSION 3.14)
project(AntiCheatEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Automatic JSON Library download
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# 2. Source files
set(SOURCES
    src/main.cpp
    src/Server.cpp
    src/PlayerManager.cpp
    src/Checks.cpp
)

# 3. Executable
add_executable(AntiCheatEngine ${SOURCES})

# 4. Target properties and linking
target_include_directories(AntiCheatEngine PRIVATE src)
target_link_libraries(AntiCheatEngine PRIVATE nlohmann_json::nlohmann_json)

# Set output name to Engine.exe
set_target_properties(AntiCheatEngine PROPERTIES OUTPUT_NAME "Engine")


# 5. Cross-platform threading and networking
find_package(Threads REQUIRED)
target_link_libraries(AntiCheatEngine PRIVATE Threads::Threads)

if(WIN32)
    target_link_libraries(AntiCheatEngine PRIVATE ws2_32)
endif()
