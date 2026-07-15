@echo off
setlocal
REM Build d3d12_saxpy.exe with VS Build Tools + Windows SDK.
set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" (
  echo error: vcvars64.bat not found at "%VCVARS%"
  echo Install VS 2022 Build Tools with C++ workload.
  exit /b 1
)

call "%VCVARS%" >nul
if errorlevel 1 exit /b 1

cd /d "%~dp0"
cl /nologo /EHsc /std:c++17 /O2 /W3 /Fe:d3d12_saxpy.exe d3d12_saxpy.cpp /link d3d12.lib dxgi.lib ole32.lib
if errorlevel 1 exit /b 1
echo Built %~dp0d3d12_saxpy.exe
endlocal
