@echo off
setlocal
cd /d "%~dp0"

set "PATH=C:\Users\niyat\New folder\dcompute-spike\ldc2-1.41.0-windows-multilib\bin;%PATH%"

echo === 1/4 build dx-compute-kit ===
call dub build
if errorlevel 1 exit /b 1

echo === 2/4 compile HLSL -^> DXIL ===
.\dx-compute-kit.exe --compile-kernel
if errorlevel 1 exit /b 1

echo === 3/4 build D3D12 host ===
call host\build.bat
if errorlevel 1 exit /b 1

echo === 4/4 run D3D12 saxpy E2E ===
host\d3d12_saxpy.exe kernels\out\saxpy.dxil
set ERR=%ERRORLEVEL%
if not "%ERR%"=="0" exit /b %ERR%

echo.
echo === fixtures (IR shape checks) ===
.\dx-compute-kit.exe --quiet
if errorlevel 1 exit /b 1

echo.
echo E2E PASS
endlocal
