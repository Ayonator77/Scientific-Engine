@echo off

:: Check if the specific build directory exists
if not exist "build\vs2026" (
    echo --- Build Directory Missing. Running CMake Configure ---
    cmake --preset msvc
    
    :: Catch configuration errors immediately
    if %errorlevel% neq 0 (
        echo [ERROR] CMake Configuration Failed! Check vcpkg or CMakeLists.txt.
        exit /b %errorlevel%
    )
)

:: If we reach here, the project is configured. Build and run it.
echo --- Building and Executing Target ---
cmake --build build\msvc --config Debug --target run