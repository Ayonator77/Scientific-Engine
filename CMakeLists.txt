cmake_minimum_required(VERSION 3.20)
project(ScientificEngine VERSION 0.1.0 LANGUAGES CXX)

# Enforce C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Useful for VS Code IntelliSense

# Find dependencies (vcpkg handles the magic here)
find_package(SDL2 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# ---------------------------------------------------------
# 1. Engine Core Library (Physics, Math, Render Logic)
# ---------------------------------------------------------
add_library(engine_core
    src/core/Application.cpp
    src/core/Camera.cpp
    src/geometry/Icosahedron.cpp
    # We will add more source files here as we build them
)

# Specify where the public headers live
target_include_directories(engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link dependencies privately to the core library
target_link_libraries(engine_core PUBLIC SDL2::SDL2main SDL2::SDL2 glad::glad glm::glm)

# ---------------------------------------------------------
# 2. Main Executable (The actual simulation app)
# ---------------------------------------------------------
add_executable(HydrostaticSim src/main.cpp)

# Link the core library to the executable
target_link_libraries(HydrostaticSim PRIVATE engine_core)

# ---------------------------------------------------------
# 4. Custom Run Target
# ---------------------------------------------------------
add_custom_target(run
    COMMAND $<TARGET_FILE:HydrostaticSim>
    DEPENDS HydrostaticSim
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ---------------------------------------------------------
# 3. Unit Tests (GTest)
# ---------------------------------------------------------
enable_testing()
add_executable(engine_tests tests/test_main.cpp)
target_link_libraries(engine_tests PRIVATE engine_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(engine_tests)
