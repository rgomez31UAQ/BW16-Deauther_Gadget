@echo off
title Image Uploader Build Tool By Warwick

echo ================================
echo  Image Uploader Build Tool By Warwick
echo ================================
echo.

REM ===== Python check =====
python --version
if errorlevel 1 (
    echo [!] Python not found
    pause
    exit /b
)

echo.

REM ===== Dependency check =====
echo [*] Checking Python dependencies...

python -c "import serial" >nul 2>&1
if errorlevel 1 (
    echo [!] PySerial not installed
    echo     Run: python -m pip install pyserial
    pause
    exit /b
)

python -c "import PyInstaller" >nul 2>&1
if errorlevel 1 (
    echo [!] PyInstaller not installed
    echo     Run: python -m pip install pyinstaller
    pause
    exit /b
)

python -c "import PyQt5" >nul 2>&1
if errorlevel 1 (
    echo [!] PyQt5 not installed
    echo     Run: python -m pip install pyqt5
    pause
    exit /b
)

echo [+] All dependencies OK
echo.

REM ===== Clean =====
echo [*] Cleaning old build...
rmdir /s /q build >nul 2>&1
rmdir /s /q dist >nul 2>&1
del /q image_uploader.spec >nul 2>&1

echo.

REM ===== Build =====
echo [*] Building EXE...
python -m PyInstaller --onefile --noconsole --name image_uploader --icon input.ico --add-data "realtek_tools;realtek_tools" main.py

if errorlevel 1 (
    echo.
    echo [!] Build failed
    pause
    exit /b
)

echo.
echo [+] Build success!
echo Output: dist\image_uploader.exe
echo.

pause
