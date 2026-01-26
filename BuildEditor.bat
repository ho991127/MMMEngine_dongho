@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ENGINE_DIR=%~dp0"
if "%ENGINE_DIR:~-1%"=="\" set "ENGINE_DIR=%ENGINE_DIR:~0,-1%"

set "MMMENGINE_DIR=%ENGINE_DIR%"

set "PLATFORM=x64"
set "SLN=%ENGINE_DIR%\MMMEngine.sln"

if not exist "%SLN%" (
  echo ERROR: Solution not found:
  echo   %SLN%
  pause
  exit /b 1
)

echo ===============================
echo   MMMEngine Editor Build
echo ===============================
echo.
echo   1) Release (recommended)
echo   2) Debug
echo.
choice /c 12 /n /m "Select build configuration: "

if errorlevel 2 set "CONFIG=Debug"
if errorlevel 1 set "CONFIG=Release"

echo.
echo Building configuration: %CONFIG%
echo.

rem --- find MSBuild ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo ERROR: vswhere not found
  pause
  exit /b 1
)

set "MSBUILD="
for /f "usebackq delims=" %%i in (
  `"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`
) do (
  set "MSBUILD=%%i"
  goto :msbuild_found
)

:msbuild_found
if not exist "%MSBUILD%" (
  echo ERROR: MSBuild not found
  pause
  exit /b 1
)

echo MSBuild:
echo   %MSBUILD%
echo.

"%MSBUILD%" "%SLN%" ^
  /t:Build ^
  /p:Platform=%PLATFORM% ^
  /p:Configuration=%CONFIG% ^
  /m:1 /v:minimal /nologo

if errorlevel 1 (
  echo.
  echo BUILD FAILED
  pause
  exit /b 1
)

rem ============================================================
rem NORMALIZED OUTPUT:
rem   - Build outputs stay in:   %ENGINE_DIR%\x64\%CONFIG%
rem   - Copy ONLY 3rd-party DLLs from Common\Bin\%CONFIG% -> x64\%CONFIG%
rem ============================================================

set "OUTDIR=%ENGINE_DIR%\%PLATFORM%\%CONFIG%"
set "THIRDPARTY=%ENGINE_DIR%\Common\Bin\%CONFIG%"

if not exist "%OUTDIR%" (
  echo ERROR: Build output folder not found:
  echo   %OUTDIR%
  pause
  exit /b 1
)

echo.
echo Copying third-party DLLs:
echo   %THIRDPARTY%
echo   -> %OUTDIR%
echo.

if exist "%THIRDPARTY%" (
  rem DLL만 복사 (서드파티 보관소 성격 유지)
  robocopy "%THIRDPARTY%" "%OUTDIR%" *.dll /NFL /NDL /NJH /NJS /NP /R:3 /W:1
  if %ERRORLEVEL% GEQ 8 (
    echo THIRD-PARTY COPY FAILED
    pause
    exit /b %ERRORLEVEL%
  )
) else (
  echo WARN: Third-party folder not found (skipped):
  echo   %THIRDPARTY%
)

echo.
echo DONE (%CONFIG%)
pause
exit /b 0
