@echo off
chcp 936 >nul
setlocal

:: 始终输出 snake.exe（覆盖旧文件）
if not "%~1"=="" (
    set "CC=%~1"
) else (
    set "CC="
    if exist "C:\TDM-GCC-64\bin\g++.exe" set "CC=C:\TDM-GCC-64\bin\g++.exe"
    if not defined CC if exist "C:\Program Files\mingw64\bin\g++.exe" set "CC=C:\Program Files\mingw64\bin\g++.exe"
    if not defined CC set "CC=g++"
)

:: 兼容常见 easyx 头文件目录
if "%EASYX_INC%"=="" (
    if exist "C:\TDM-GCC-64\x86_64-w64-mingw32\include\easyx.h" (
        set "EASYX_INC=C:\TDM-GCC-64\x86_64-w64-mingw32\include"
    ) else if exist "C:\Program Files\mingw64\include\easyx.h" (
        set "EASYX_INC=C:\Program Files\mingw64\include"
    ) else (
        echo [错误] 未找到 easyx.h，请先设置 EASYX_INC 环境变量为 easyx 头文件目录。
        exit /b 1
    )
)

"%CC%" -I"%EASYX_INC%" -O2 -std=c++11 -finput-charset=UTF-8 -fexec-charset=CP936 *.cpp -o snake.exe ^
    -leasyx -lwinmm -lgdi32 -luser32 -lkernel32 -lws2_32

if errorlevel 1 (
    echo [失败] 编译未通过，请确认 MinGW/EasyX 环境与源文件编码。
    exit /b 1
)

echo [成功] 已生成 snake.exe（已覆盖同名旧文件）
exit /b 0
