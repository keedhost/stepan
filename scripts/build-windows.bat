@echo off
REM Stepan — твій друг в системі координат
REM Copyright (C) 2026 Andrii Kondratiev
REM
REM Слава Україні! Слава ЗСУ!
REM
REM SPDX-License-Identifier: GPL-3.0-or-later
REM
REM This program is free software: you can redistribute it and/or modify
REM it under the terms of the GNU General Public License as published by
REM the Free Software Foundation, either version 3 of the License, or
REM (at your option) any later version.
REM
REM This program is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with this program. If not, see <https://www.gnu.org/licenses/>.

setlocal EnableDelayedExpansion

REM ─────────────────────────────────────────────────────────────
REM  Stepan — Windows build script (x64)
REM  Requires: Qt6, CMake, Visual Studio 2019/2022 or Ninja+MinGW
REM ─────────────────────────────────────────────────────────────

set "ROOT_DIR=%~dp0.."
set "BUILD_DIR=%ROOT_DIR%\build-windows"

REM ── Locate Qt6 ───────────────────────────────────────────────
if not defined CMAKE_PREFIX_PATH (
    REM Try common install paths
    for %%P in (
        "C:\Qt\6.7.0\msvc2019_64"
        "C:\Qt\6.7.0\msvc2022_64"
        "C:\Qt\6.6.0\msvc2019_64"
        "C:\Qt\6.6.0\msvc2022_64"
        "C:\Qt\6.5.0\msvc2019_64"
        "%USERPROFILE%\Qt\6.7.0\msvc2022_64"
    ) do (
        if exist %%P (
            set "CMAKE_PREFIX_PATH=%%~P"
            echo Found Qt6 at: %%~P
            goto :qt_found
        )
    )
    echo ERROR: Qt6 not found. Please set CMAKE_PREFIX_PATH, e.g.:
    echo   set CMAKE_PREFIX_PATH=C:\Qt\6.7.0\msvc2022_64
    exit /b 1
    :qt_found
)

REM ── Check for CMake ──────────────────────────────────────────
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: cmake not found. Install from https://cmake.org/download/
    exit /b 1
)

REM ── Detect generator ─────────────────────────────────────────
set "GENERATOR=Visual Studio 17 2022"
where ninja >nul 2>&1
if not errorlevel 1 (
    set "GENERATOR=Ninja"
)

echo =^> Using generator: %GENERATOR%

REM ── Configure ────────────────────────────────────────────────
echo =^> Configuring...
if "%GENERATOR%"=="Ninja" (
    cmake -B "%BUILD_DIR%" -G "Ninja" ^
          -DCMAKE_BUILD_TYPE=Release ^
          -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" ^
          -S "%ROOT_DIR%"
) else (
    cmake -B "%BUILD_DIR%" -G "%GENERATOR%" -A x64 ^
          -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" ^
          -S "%ROOT_DIR%"
)
if errorlevel 1 (
    echo CMake configure failed.
    exit /b 1
)

REM ── Build ────────────────────────────────────────────────────
echo =^> Building...
cmake --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

REM ── Deploy Qt DLLs ───────────────────────────────────────────
if "%GENERATOR%"=="Ninja" (
    set "EXE_DIR=%BUILD_DIR%"
) else (
    set "EXE_DIR=%BUILD_DIR%\Release"
)

set "EXE=%EXE_DIR%\Stepan.exe"
set "WINDEPLOYQT=%CMAKE_PREFIX_PATH%\bin\windeployqt.exe"

if exist "%EXE%" (
    if exist "%WINDEPLOYQT%" (
        echo =^> Running windeployqt...
        "%WINDEPLOYQT%" --release "%EXE%"
    ) else (
        echo windeployqt not found at %WINDEPLOYQT% — skipping Qt DLL deployment.
    )
    echo.
    echo Build complete.
    echo   Executable: %EXE%
) else (
    echo Build output not found at expected path: %EXE%
)

endlocal
