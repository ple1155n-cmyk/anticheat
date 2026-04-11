@echo off
setlocal enabledelayedexpansion

:: ========================================
::   Anti-Cheat Engine Build Script (Auto-MSVC Edition)
:: ========================================

set "PROJECT_DIR=%~dp0"
set "BUILD_DIR=%PROJECT_DIR%build"
set "RESOURCE_DIR=%PROJECT_DIR%..\src\main\resources"

echo ========================================
echo   Building Anti-Cheat Engine
echo ========================================
echo [INFO] Searching for Visual Studio 2022 Environment...

:: Tự động nạp môi trường C++ của Microsoft vào Terminal của Antigravity
set "VCVARS=D:\vscodeC++\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VCVARS%" (
    call "%VCVARS%" >nul
    echo [INFO] MSVC Environment loaded successfully!
) else (
    echo [WARNING] Visual Studio C++ Workload not found at default location.
    echo Please ensure Visual Studio 2022 Community is installed.
)

:: 1. Tạo thư mục build
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

:: 2. Chạy cấu hình CMake (Đã sửa lỗi đường dẫn)
echo [INFO] Configuring CMake...
cmake -S . -B "%BUILD_DIR%" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_POLICY_DEFAULT_CMP0135=NEW

if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed.
    goto :EXIT
)

:: 3. Tiến hành đúc file .exe (Release Mode)
echo [INFO] Building Release configuration...
cmake --build "%BUILD_DIR%" --config Release --parallel

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed.
    goto :EXIT
)

:: 4. Tự động Copy vào Java
echo [INFO] Deploying Engine.exe to Java resources...
set "SOURCE_EXE="
if exist "%BUILD_DIR%\Release\Engine.exe" (
    set "SOURCE_EXE=%BUILD_DIR%\Release\Engine.exe"
) else if exist "%BUILD_DIR%\Engine.exe" (
    set "SOURCE_EXE=%BUILD_DIR%\Engine.exe"
)

if defined SOURCE_EXE (
    copy /Y "%SOURCE_EXE%" "%RESOURCE_DIR%\Engine.exe"
    echo [SUCCESS] Engine.exe deployed to %RESOURCE_DIR%
) else (
    echo [ERROR] Could not find Engine.exe in build artifacts.
)

:EXIT
echo ========================================
pause