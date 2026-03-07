@echo off
echo --- Cleaning Build Directory ---
if exist "build" rmdir /s /q "build"
if exist ".cache" rmdir /s /q ".cache"
echo --- Clean Complete ---