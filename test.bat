@echo off
echo =========================================
echo       BUILDING V^&V TEST SUITE
echo =========================================
cmake --build build/msvc --target engine_tests

if %errorlevel% neq 0 (
    echo.
    echo [FATAL] Build failed. Tests aborted.
    exit /b %errorlevel%
)

echo.
echo =========================================
echo       EXECUTING V^&V TEST SUITE
echo =========================================
.\build\msvc\Debug\engine_tests.exe